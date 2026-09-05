#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "wifiiot_errno.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "wifiiot_uart.h"

/*
 * Hi3861 autonomous-track controller.
 *
 * Stable facts used here:
 *   - GPIO13 = left digital IR sensor.
 *   - GPIO14 = right digital IR sensor.
 *   - Current hardware polarity:
 *       0 = white border
 *       1 = black tape / outside white border
 *   - GPIO11 / UART2_TX -> STM32 PA10 / USART1_RX.
 *   - UART = 115200-8-N-1.
 *   - Motor frame = FC dirL speedL dirR speedR FD.
 *   - STM32 is treated as an already validated speed-target executor
 *     with an independent ~2 s communication timeout.
 *
 * Important limitation:
 *
 * Two binary IR sensors cannot uniquely distinguish every Y junction,
 * dead-end transverse bar, and full start/finish marker from one sample.
 *
 * Therefore the navigation layer combines:
 *
 *   debounced observations
 *   + event timing
 *   + navigation phase
 *   + bounded actions
 *   + timeout protection
 *
 * Geometry-dependent constants below are calibration parameters,
 * not established protocol facts.
 */

/* -------------------------------------------------------------------------- */
/* Hardware contract                                                          */
/* -------------------------------------------------------------------------- */

#define TRACK_LEFT_SENSOR_GPIO 13
#define TRACK_RIGHT_SENSOR_GPIO 14

#define TRACK_UART_INDEX WIFI_IOT_UART_IDX_2

#define MOTOR_FRAME_HEADER 0xFCU
#define MOTOR_FRAME_TAIL 0xFDU
#define MOTOR_FRAME_LEN 6U

#define MOTOR_SPEED_MAX 250

/* -------------------------------------------------------------------------- */
/* Scheduling / filtering                                                     */
/* -------------------------------------------------------------------------- */

#define SENSOR_PERIOD_MS 20U
#define SENSOR_CONFIRM_SAMPLES 3U

#define MOTOR_TX_PERIOD_MS 80U
#define STATUS_LOG_PERIOD_MS 500U

#define STARTUP_SENSOR_TIMEOUT_MS 1000U

/* -------------------------------------------------------------------------- */
/* Motion calibration                                                        */
/* -------------------------------------------------------------------------- */

/*
 * [PENDING HW CALIBRATION]
 *
 * Signed speed units follow the existing STM32 protocol:
 *
 *     1 unit = 0.01 rad/s
 */

#define SPEED_BASE 110

#define SPEED_CORRECT_SLOW 65
#define SPEED_CORRECT_FAST 135

#define SPEED_EVENT_PROBE 80
#define SPEED_CENTER 75

#define SPEED_TURN_REVERSE 70
#define SPEED_TURN_FORWARD 140

#define SPEED_UTURN 110

#define SPEED_BACKTRACK_BASE 100
#define SPEED_BACKTRACK_SLOW 60
#define SPEED_BACKTRACK_FAST 125

/* -------------------------------------------------------------------------- */
/* Event timing                                                               */
/* -------------------------------------------------------------------------- */

/*
 * A stable 11 interval is treated as a candidate wide-black event.
 *
 * [PENDING HW CALIBRATION]
 */

#define WIDE_PULSE_MIN_MS 80U
#define WIDE_PULSE_MAX_MS 2000U

/*
 * Full start / finish marker:
 *
 *     first wide-black pulse
 *         ->
 *     gap / longitudinal line phase
 *         ->
 *     second wide-black pulse
 */

#define MARKER_GAP_MIN_MS 80U
#define MARKER_GAP_MAX_MS 550U

/* Junction centering. */

#define CENTER_FORWARD_MS 240U
#define CENTER_ACTION_TIMEOUT_MS 700U

/* Turn recognition. */

#define TURN_MIN_MS 220U
#define TURN_TIMEOUT_MS 1700U
#define TURN_CENTER_HOLD_SAMPLES 4U

/* U-turn recognition. */

#define UTURN_MIN_MS 650U
#define UTURN_TIMEOUT_MS 2600U
#define UTURN_CENTER_HOLD_SAMPLES 5U

/* General safety bounds. */

#define BACKTRACK_TIMEOUT_MS 20000U

/* Current track requirement: two fixed-choice Y junctions. */

#define REQUIRED_FIXED_JUNCTIONS 2U

#define PATH_STACK_CAPACITY 8U

/* -------------------------------------------------------------------------- */
/* Types                                                                      */
/* -------------------------------------------------------------------------- */

typedef enum {
  OBS_00 = 0,
  OBS_10,
  OBS_01,
  OBS_11,
  OBS_INVALID
} SensorObservation;

typedef enum {
  NAV_WAIT_SENSOR = 0,

  NAV_FOLLOW_LINE,

  NAV_CENTER_JUNCTION,
  NAV_EXECUTE_TURN,

  NAV_U_TURN,
  NAV_BACKTRACK,

  NAV_FINISHED,
  NAV_FAULT
} NavState;

typedef enum { TURN_LEFT = 0, TURN_RIGHT } TurnDirection;

typedef enum {
  TURN_PURPOSE_NEW_JUNCTION = 0,

  TURN_PURPOSE_BACKTRACK_ALTERNATE,

  TURN_PURPOSE_BACKTRACK_PARENT
} TurnPurpose;

typedef enum {
  MARK_IDLE = 0,

  MARK_FIRST_BLACK,

  MARK_GAP,

  MARK_SECOND_BLACK
} MarkerPhase;

typedef enum {
  TRACK_EVENT_NONE = 0,

  TRACK_EVENT_SINGLE_BAND,

  TRACK_EVENT_DOUBLE_BAND,

  TRACK_EVENT_AMBIGUOUS_FAULT
} TrackEvent;

typedef struct {
  SensorObservation candidate;
  SensorObservation stable;

  uint8_t candidateCount;
} SensorFilter;

typedef struct {
  MarkerPhase phase;

  uint32_t phaseSinceMs;
} MarkerDetector;

typedef struct {
  uint8_t ordinal;

  TurnDirection preferredTurn;

  bool alternateTried;
} JunctionNode;

/* -------------------------------------------------------------------------- */
/* Global navigation state                                                    */
/* -------------------------------------------------------------------------- */

static NavState g_state = NAV_WAIT_SENSOR;

static SensorFilter g_filter = {OBS_INVALID, OBS_INVALID, 0U};

static MarkerDetector g_marker = {MARK_IDLE, 0U};

static JunctionNode g_pathStack[PATH_STACK_CAPACITY];

static uint8_t g_pathDepth = 0U;
static uint8_t g_uniqueJunctions = 0U;

static bool g_startMarkerSeen = false;

/* Current requested motor target. */

static int16_t g_targetLeft = 0;
static int16_t g_targetRight = 0;

static bool g_targetDirty = true;

/* Timing. */

static uint32_t g_stateSinceMs = 0U;

static uint32_t g_lastValidTrackingMs = 0U;

static uint32_t g_lastMotorTxMs = 0U;
static uint32_t g_lastStatusLogMs = 0U;

static uint32_t g_backtrackSinceMs = 0U;

static uint32_t g_sampleNumber = 0U;

/* Current turn action. */

static TurnDirection g_pendingTurn = TURN_LEFT;

static TurnPurpose g_turnPurpose = TURN_PURPOSE_NEW_JUNCTION;

static uint8_t g_pendingJunctionOrdinal = 0U;

static bool g_turnSawDeviation = false;
static uint8_t g_turnCenterCount = 0U;

/* Current U-turn action. */

static bool g_uturnSawDeviation = false;
static uint8_t g_uturnCenterCount = 0U;

/* -------------------------------------------------------------------------- */
/* Time utilities                                                             */
/* -------------------------------------------------------------------------- */

static uint32_t NowMs(void) {
  uint32_t frequency;
  uint32_t ticks;

  frequency = osKernelGetTickFreq();
  ticks = osKernelGetTickCount();

  if (frequency == 0U) {
    return 0U;
  }

  return (uint32_t)(((uint64_t)ticks * 1000ULL) / (uint64_t)frequency);
}

static uint32_t ElapsedMs(uint32_t nowMs, uint32_t sinceMs) {
  return nowMs - sinceMs;
}

/* -------------------------------------------------------------------------- */
/* Debug names                                                                */
/* -------------------------------------------------------------------------- */

static const char *StateName(NavState state) {
  switch (state) {
  case NAV_WAIT_SENSOR:
    return "WAIT_SENSOR";

  case NAV_FOLLOW_LINE:
    return "FOLLOW_LINE";

  case NAV_CENTER_JUNCTION:
    return "CENTER_JUNCTION";

  case NAV_EXECUTE_TURN:
    return "EXECUTE_TURN";

  case NAV_U_TURN:
    return "U_TURN";

  case NAV_BACKTRACK:
    return "BACKTRACK";

  case NAV_FINISHED:
    return "FINISHED";

  case NAV_FAULT:
    return "FAULT";

  default:
    return "UNKNOWN";
  }
}

static const char *ObservationName(SensorObservation observation) {
  switch (observation) {
  case OBS_00:
    return "00";

  case OBS_10:
    return "10";

  case OBS_01:
    return "01";

  case OBS_11:
    return "11";

  default:
    return "INVALID";
  }
}

static const char *TurnName(TurnDirection turn) {
  return (turn == TURN_LEFT) ? "LEFT" : "RIGHT";
}

static TurnDirection OppositeTurn(TurnDirection turn) {
  return (turn == TURN_LEFT) ? TURN_RIGHT : TURN_LEFT;
}

/* -------------------------------------------------------------------------- */
/* State transition                                                           */
/* -------------------------------------------------------------------------- */

static void SetState(NavState next, uint32_t nowMs, const char *reason) {
  if (g_state != next) {
    printf("TRACK,STATE,%s->%s,REASON=%s\r\n", StateName(g_state),
           StateName(next), reason);
  }

  g_state = next;
  g_stateSinceMs = nowMs;
}

/* -------------------------------------------------------------------------- */
/* Motor link                                                                 */
/* -------------------------------------------------------------------------- */

static int16_t ClampSignedSpeed(int16_t speed) {
  if (speed > MOTOR_SPEED_MAX) {
    return MOTOR_SPEED_MAX;
  }

  if (speed < -MOTOR_SPEED_MAX) {
    return -MOTOR_SPEED_MAX;
  }

  return speed;
}

static void EncodeMotorFrame(int16_t left, int16_t right,
                             unsigned char frame[MOTOR_FRAME_LEN]) {
  int leftValue;
  int rightValue;

  left = ClampSignedSpeed(left);
  right = ClampSignedSpeed(right);

  leftValue = (int)left;
  rightValue = (int)right;

  frame[0] = MOTOR_FRAME_HEADER;

  frame[1] = (leftValue < 0) ? 1U : 0U;

  frame[2] = (unsigned char)((leftValue < 0) ? -leftValue : leftValue);

  frame[3] = (rightValue < 0) ? 1U : 0U;

  frame[4] = (unsigned char)((rightValue < 0) ? -rightValue : rightValue);

  frame[5] = MOTOR_FRAME_TAIL;
}

static int WriteMotorTarget(int16_t left, int16_t right) {
  unsigned char frame[MOTOR_FRAME_LEN];

  int written;

  EncodeMotorFrame(left, right, frame);

  written = UartWrite(TRACK_UART_INDEX, frame, MOTOR_FRAME_LEN);

  return (written == (int)MOTOR_FRAME_LEN) ? 0 : -1;
}

static void SetMotorTarget(int16_t left, int16_t right) {
  left = ClampSignedSpeed(left);
  right = ClampSignedSpeed(right);

  if (left != g_targetLeft || right != g_targetRight) {
    g_targetLeft = left;
    g_targetRight = right;

    g_targetDirty = true;

    printf("TRACK,MOTOR_TARGET,L=%d,R=%d\r\n", (int)left, (int)right);
  }
}

static void StopMotorTarget(void) { SetMotorTarget(0, 0); }

static int RefreshMotorIfNeeded(uint32_t nowMs) {
  if (!g_targetDirty &&
      ElapsedMs(nowMs, g_lastMotorTxMs) < MOTOR_TX_PERIOD_MS) {
    return 0;
  }

  if (WriteMotorTarget(g_targetLeft, g_targetRight) != 0) {
    /*
     * Best-effort STOP.
     *
     * STM32 independently stops after
     * approximately two seconds without a
     * valid command.
     */

    (void)WriteMotorTarget(0, 0);

    return -1;
  }

  g_lastMotorTxMs = nowMs;

  g_targetDirty = false;

  return 0;
}

/* -------------------------------------------------------------------------- */
/* Sensor acquisition                                                         */
/* -------------------------------------------------------------------------- */

static int ReadRawSensors(WifiIotGpioValue *left, WifiIotGpioValue *right) {
  int leftResult;
  int rightResult;

  if (left == NULL || right == NULL) {
    return -1;
  }

  leftResult = GpioGetInputVal(TRACK_LEFT_SENSOR_GPIO, left);

  rightResult = GpioGetInputVal(TRACK_RIGHT_SENSOR_GPIO, right);

  if (leftResult != WIFI_IOT_SUCCESS || rightResult != WIFI_IOT_SUCCESS) {
    printf("TRACK,SENSOR_READ_ERROR,"
           "L_RC=%d,R_RC=%d\r\n",
           leftResult, rightResult);

    return -1;
  }

  return 0;
}

static SensorObservation ObservationFromRaw(WifiIotGpioValue left,
                                            WifiIotGpioValue right) {
  if (left == WIFI_IOT_GPIO_VALUE0 && right == WIFI_IOT_GPIO_VALUE0) {
    return OBS_00;
  }

  if (left == WIFI_IOT_GPIO_VALUE1 && right == WIFI_IOT_GPIO_VALUE0) {
    return OBS_10;
  }

  if (left == WIFI_IOT_GPIO_VALUE0 && right == WIFI_IOT_GPIO_VALUE1) {
    return OBS_01;
  }

  if (left == WIFI_IOT_GPIO_VALUE1 && right == WIFI_IOT_GPIO_VALUE1) {
    return OBS_11;
  }

  return OBS_INVALID;
}

static SensorObservation FilterObservation(SensorObservation raw) {
  if (raw == OBS_INVALID) {
    return OBS_INVALID;
  }

  if (g_filter.candidate == raw) {
    if (g_filter.candidateCount < 255U) {
      ++g_filter.candidateCount;
    }
  } else {
    g_filter.candidate = raw;
    g_filter.candidateCount = 1U;
  }

  if (g_filter.candidateCount >= SENSOR_CONFIRM_SAMPLES) {
    g_filter.stable = raw;
  }

  return g_filter.stable;
}

/* -------------------------------------------------------------------------- */
/* Marker detector                                                            */
/* -------------------------------------------------------------------------- */

static void ResetMarkerDetector(void) {
  g_marker.phase = MARK_IDLE;

  g_marker.phaseSinceMs = 0U;
}

static bool MarkerDetectorActive(void) { return g_marker.phase != MARK_IDLE; }

static TrackEvent UpdateMarkerDetector(SensorObservation observation,
                                       uint32_t nowMs) {
  uint32_t elapsed;

  switch (g_marker.phase) {

  case MARK_IDLE:

    if (observation == OBS_11) {
      g_marker.phase = MARK_FIRST_BLACK;

      g_marker.phaseSinceMs = nowMs;

      printf("TRACK,EVENT_CANDIDATE,"
             "WIDE_BLACK_ENTER\r\n");
    }

    return TRACK_EVENT_NONE;

  case MARK_FIRST_BLACK:

    elapsed = ElapsedMs(nowMs, g_marker.phaseSinceMs);

    if (observation == OBS_11) {

      if (elapsed > WIDE_PULSE_MAX_MS) {
        ResetMarkerDetector();

        return TRACK_EVENT_AMBIGUOUS_FAULT;
      }

      return TRACK_EVENT_NONE;
    }

    if (elapsed < WIDE_PULSE_MIN_MS) {
      /*
       * Too short after debounce:
       * transient edge contact.
       */

      ResetMarkerDetector();

      return TRACK_EVENT_NONE;
    }

    g_marker.phase = MARK_GAP;

    g_marker.phaseSinceMs = nowMs;

    return TRACK_EVENT_NONE;

  case MARK_GAP:

    elapsed = ElapsedMs(nowMs, g_marker.phaseSinceMs);

    if (observation == OBS_11) {

      if (elapsed < MARKER_GAP_MIN_MS) {
        /*
         * Probably re-entry / bounce
         * into the first black band.
         */

        g_marker.phase = MARK_FIRST_BLACK;

        g_marker.phaseSinceMs = nowMs;

        return TRACK_EVENT_NONE;
      }

      if (elapsed <= MARKER_GAP_MAX_MS) {
        g_marker.phase = MARK_SECOND_BLACK;

        g_marker.phaseSinceMs = nowMs;

        return TRACK_EVENT_NONE;
      }
    }

    if (elapsed > MARKER_GAP_MAX_MS) {
      ResetMarkerDetector();

      return TRACK_EVENT_SINGLE_BAND;
    }

    return TRACK_EVENT_NONE;

  case MARK_SECOND_BLACK:

    elapsed = ElapsedMs(nowMs, g_marker.phaseSinceMs);

    if (observation == OBS_11) {

      if (elapsed > WIDE_PULSE_MAX_MS) {
        ResetMarkerDetector();

        return TRACK_EVENT_AMBIGUOUS_FAULT;
      }

      return TRACK_EVENT_NONE;
    }

    if (elapsed >= WIDE_PULSE_MIN_MS) {
      ResetMarkerDetector();

      return TRACK_EVENT_DOUBLE_BAND;
    }

    /*
     * Short second contact:
     * probably noise.
     */

    g_marker.phase = MARK_GAP;

    g_marker.phaseSinceMs = nowMs;

    return TRACK_EVENT_NONE;

  default:

    ResetMarkerDetector();

    return TRACK_EVENT_AMBIGUOUS_FAULT;
  }
}

/* -------------------------------------------------------------------------- */
/* Safety                                                                     */
/* -------------------------------------------------------------------------- */

static void EnterFault(const char *reason, uint32_t nowMs) {
  StopMotorTarget();

  ResetMarkerDetector();

  SetState(NAV_FAULT, nowMs, reason);

  printf("TRACK,FAULT,%s\r\n", reason);
}



/* -------------------------------------------------------------------------- */
/* Line-follow policy                                                         */
/* -------------------------------------------------------------------------- */

static void ApplyLineFollowTarget(SensorObservation observation,
                                  bool backtracking) {
  int16_t base;
  int16_t slow;
  int16_t fast;

  base = backtracking ? SPEED_BACKTRACK_BASE : SPEED_BASE;

  slow = backtracking ? SPEED_BACKTRACK_SLOW : SPEED_CORRECT_SLOW;

  fast = backtracking ? SPEED_BACKTRACK_FAST : SPEED_CORRECT_FAST;

  switch (observation) {

  case OBS_00:

    SetMotorTarget(base, base);

    break;

  case OBS_10:

    /*
     * Left sensor sees black:
     *
     * left wheel slower
     * right wheel faster
     *
     * -> steer left
     */

    SetMotorTarget(slow, fast);

    break;

  case OBS_01:

    /*
     * Right sensor sees black:
     *
     * left wheel faster
     * right wheel slower
     *
     * -> steer right
     */

    SetMotorTarget(fast, slow);

    break;

  case OBS_11:

    /*
     * 11 is NOT directly interpreted
     * as junction/dead-end/finish.
     *
     * Temporal event detector owns
     * interpretation.
     */

    SetMotorTarget(SPEED_EVENT_PROBE, SPEED_EVENT_PROBE);

    break;

  default:

    StopMotorTarget();

    break;
  }
}

/* -------------------------------------------------------------------------- */
/* Junction/path helpers                                                      */
/* -------------------------------------------------------------------------- */

static TurnDirection FixedTurnForOrdinal(uint8_t ordinal) {
  /*
   * Required fixed route:
   *
   * Junction 1 -> left
   * Junction 2 -> right
   */

  return (ordinal == 1U) ? TURN_LEFT : TURN_RIGHT;
}

static void BeginTurn(TurnDirection turn, TurnPurpose purpose, uint8_t ordinal,
                      uint32_t nowMs) {
  g_pendingTurn = turn;

  g_turnPurpose = purpose;

  g_pendingJunctionOrdinal = ordinal;

  g_turnSawDeviation = false;

  g_turnCenterCount = 0U;

  ResetMarkerDetector();

  SetState(NAV_EXECUTE_TURN, nowMs, "CENTER_COMPLETE");

  if (turn == TURN_LEFT) {

    SetMotorTarget(SPEED_TURN_REVERSE, SPEED_TURN_FORWARD);

  } else {

    SetMotorTarget(SPEED_TURN_FORWARD, SPEED_TURN_REVERSE);
  }

  printf("TRACK,TURN_BEGIN,"
         "DIR=%s,PURPOSE=%u,ORDINAL=%u\r\n",
         TurnName(turn), (unsigned int)purpose, (unsigned int)ordinal);
}

static void BeginCenterForTurn(TurnDirection turn, TurnPurpose purpose,
                               uint8_t ordinal, uint32_t nowMs) {
  g_pendingTurn = turn;

  g_turnPurpose = purpose;

  g_pendingJunctionOrdinal = ordinal;

  ResetMarkerDetector();

  SetMotorTarget(SPEED_CENTER, SPEED_CENTER);

  SetState(NAV_CENTER_JUNCTION, nowMs, "SINGLE_BAND_CONFIRMED");

  printf("TRACK,JUNCTION_CENTER,"
         "DIR=%s,PURPOSE=%u,ORDINAL=%u\r\n",
         TurnName(turn), (unsigned int)purpose, (unsigned int)ordinal);
}

static void BeginUTurn(uint32_t nowMs, const char *reason) {
  ResetMarkerDetector();

  g_uturnSawDeviation = false;

  g_uturnCenterCount = 0U;

  SetState(NAV_U_TURN, nowMs, reason);

  /*
   * U-turn is bounded.
   *
   * Ordinary line following never uses
   * reverse/spin recovery.
   */

  SetMotorTarget(-SPEED_UTURN, SPEED_UTURN);

  printf("TRACK,UTURN_BEGIN,REASON=%s\r\n", reason);
}

static int PushJunction(uint8_t ordinal, TurnDirection preferred) {
  JunctionNode *node;

  if (g_pathDepth >= PATH_STACK_CAPACITY) {
    return -1;
  }

  node = &g_pathStack[g_pathDepth++];

  node->ordinal = ordinal;

  node->preferredTurn = preferred;

  node->alternateTried = false;

  return 0;
}

static void CompleteTurn(uint32_t nowMs) {
  switch (g_turnPurpose) {

  case TURN_PURPOSE_NEW_JUNCTION:

    if (PushJunction(g_pendingJunctionOrdinal, g_pendingTurn) != 0) {
      EnterFault("PATH_STACK_FULL", nowMs);

      return;
    }

    if (g_pendingJunctionOrdinal > g_uniqueJunctions) {
      g_uniqueJunctions = g_pendingJunctionOrdinal;
    }

    printf("TRACK,JUNCTION_COMMIT,"
           "ORDINAL=%u,TURN=%s,DEPTH=%u\r\n",
           (unsigned int)g_pendingJunctionOrdinal, TurnName(g_pendingTurn),
           (unsigned int)g_pathDepth);

    g_lastValidTrackingMs = nowMs;

    SetState(NAV_FOLLOW_LINE, nowMs, "TURN_REACQUIRED_LINE");

    break;

  case TURN_PURPOSE_BACKTRACK_ALTERNATE:

    printf("TRACK,BACKTRACK_EXIT,"
           "ALTERNATE_BRANCH\r\n");

    g_lastValidTrackingMs = nowMs;

    SetState(NAV_FOLLOW_LINE, nowMs, "ALTERNATE_BRANCH_SELECTED");

    break;

  case TURN_PURPOSE_BACKTRACK_PARENT:

    printf("TRACK,BACKTRACK_EXIT,"
           "PARENT_PATH\r\n");

    g_lastValidTrackingMs = nowMs;

    SetState(NAV_BACKTRACK, nowMs, "RETURN_TO_PARENT_PATH");

    break;

  default:

    EnterFault("INVALID_TURN_PURPOSE", nowMs);

    break;
  }
}

/* -------------------------------------------------------------------------- */
/* Track events                                                               */
/* -------------------------------------------------------------------------- */

static void HandleSingleBand(bool backtracking, uint32_t nowMs) {
  JunctionNode *node;

  TurnDirection turn;

  printf("TRACK,EVENT,SINGLE_BAND,MODE=%s\r\n",
         backtracking ? "BACKTRACK" : "FORWARD");

  if (backtracking) {

    if (g_pathDepth == 0U) {

      EnterFault("BACKTRACK_WITH_EMPTY_PATH", nowMs);

      return;
    }

    node = &g_pathStack[g_pathDepth - 1U];

    if (!node->alternateTried) {

      node->alternateTried = true;

      /*
       * [PENDING GEOMETRY CALIBRATION]
       *
       * For a symmetric Y:
       *
       * returning from the preferred
       * branch and crossing into the
       * other branch is assumed here
       * to use the same turn sense.
       */

      turn = node->preferredTurn;

      BeginCenterForTurn(turn, TURN_PURPOSE_BACKTRACK_ALTERNATE, node->ordinal,
                         nowMs);

      return;
    }

    /*
     * Both forward exits at this node
     * were tried.
     *
     * Pop node and continue toward its
     * parent.
     */

    turn = OppositeTurn(node->preferredTurn);

    printf("TRACK,PATH_POP,"
           "ORDINAL=%u,DEPTH_BEFORE=%u\r\n",
           (unsigned int)node->ordinal, (unsigned int)g_pathDepth);

    --g_pathDepth;

    BeginCenterForTurn(turn, TURN_PURPOSE_BACKTRACK_PARENT, node->ordinal,
                       nowMs);

    return;
  }

  /*
   * Forward exploration.
   */

  if (g_uniqueJunctions < REQUIRED_FIXED_JUNCTIONS) {
    uint8_t ordinal;

    ordinal = (uint8_t)(g_uniqueJunctions + 1U);

    turn = FixedTurnForOrdinal(ordinal);

    BeginCenterForTurn(turn, TURN_PURPOSE_NEW_JUNCTION, ordinal, nowMs);

    return;
  }

  /*
   * Once both fixed Y junctions are
   * already committed, an isolated
   * transverse band is interpreted as
   * a dead-end candidate.
   */

  BeginUTurn(nowMs, "DEAD_END_SINGLE_BAND");
}

static void HandleDoubleBand(bool backtracking) {
  printf("TRACK,EVENT,DOUBLE_BAND,"
         "MODE=%s,JUNCTIONS=%u\r\n",
         backtracking ? "BACKTRACK" : "FORWARD",
         (unsigned int)g_uniqueJunctions);

  if (backtracking) {

    /*
     * Marker seen while returning must
     * not terminate the mission.
     */

    printf("TRACK,MARKER_IGNORED,"
           "BACKTRACK\r\n");

    return;
  }

  /*
   * After both required Y junctions,
   * the complete two-band marker is
   * considered the finish marker.
   */

    if (
        g_uniqueJunctions >=
        REQUIRED_FIXED_JUNCTIONS
    ) {
        /*
        * Double-band is only a finish candidate.
        *
        * Do not permanently stop immediately:
        * two binary sensors can produce a similar
        * sequence around junction edges or large
        * steering oscillations.
        */
        printf(
            "TRACK,FINISH_CANDIDATE,"
            "DOUBLE_BAND_CONFIRMED\r\n"
        );

        g_startMarkerSeen = true;

        return;
    }

  /*
   * Before the first Y junction,
   * the first complete marker can be
   * interpreted as the start marker.
   */

  if (!g_startMarkerSeen && g_uniqueJunctions == 0U) {
    g_startMarkerSeen = true;

    printf("TRACK,START_MARKER,"
           "CONFIRMED\r\n");

    return;
  }

  printf("TRACK,MARKER_IGNORED,"
         "PHASE_NOT_FINISH\r\n");
}

/* -------------------------------------------------------------------------- */
/* Track following state                                                      */
/* -------------------------------------------------------------------------- */

static void ProcessTrackFollowing(SensorObservation observation,
                                  bool backtracking, uint32_t nowMs) {
  TrackEvent event;

  if (observation == OBS_INVALID) {
    EnterFault("INVALID_SENSOR_OBSERVATION", nowMs);

    return;
  }

  event = UpdateMarkerDetector(observation, nowMs);

  /*
   * Slow down while a wide-black event
   * is being classified.
   */

  if (MarkerDetectorActive()) {

    SetMotorTarget(SPEED_EVENT_PROBE, SPEED_EVENT_PROBE);

  } else {

    ApplyLineFollowTarget(observation, backtracking);
  }

  /*
   * 00 represents the nominal centered
   * line-following observation.
   */

    if (
        !MarkerDetectorActive() &&
        (
            observation == OBS_00 ||
            observation == OBS_10 ||
            observation == OBS_01
        )
    ) {
        g_lastValidTrackingMs =
            nowMs;
    }

  switch (event) {

  case TRACK_EVENT_NONE:

    break;

  case TRACK_EVENT_SINGLE_BAND:

    HandleSingleBand(backtracking, nowMs);

    break;

  case TRACK_EVENT_DOUBLE_BAND:

    HandleDoubleBand(backtracking);

    break;

  case TRACK_EVENT_AMBIGUOUS_FAULT:
  default:

    EnterFault("UNCLASSIFIED_WIDE_BLACK", nowMs);

    break;
  }

  if (g_state == NAV_FAULT || g_state == NAV_FINISHED) {
    return;
  }

  /*
   * If the vehicle cannot recover a
   * nominal centered observation for
   * too long, do not continue moving
   * indefinitely.
   */



  if (backtracking &&
      ElapsedMs(nowMs, g_backtrackSinceMs) > BACKTRACK_TIMEOUT_MS) {
    EnterFault("BACKTRACK_TIMEOUT", nowMs);
  }
}

/* -------------------------------------------------------------------------- */
/* Junction centering                                                         */
/* -------------------------------------------------------------------------- */

static void ProcessCenterState(uint32_t nowMs) {
  uint32_t elapsed;

  elapsed = ElapsedMs(nowMs, g_stateSinceMs);

  SetMotorTarget(SPEED_CENTER, SPEED_CENTER);

  if (elapsed > CENTER_ACTION_TIMEOUT_MS) {
    EnterFault("CENTER_TIMEOUT", nowMs);

    return;
  }

  if (elapsed >= CENTER_FORWARD_MS) {
    BeginTurn(g_pendingTurn, g_turnPurpose, g_pendingJunctionOrdinal, nowMs);
  }
}

/* -------------------------------------------------------------------------- */
/* Turn execution                                                             */
/* -------------------------------------------------------------------------- */

static void ProcessTurnState(SensorObservation observation, uint32_t nowMs) {
  uint32_t elapsed;

  elapsed = ElapsedMs(nowMs, g_stateSinceMs);

  if (observation == OBS_INVALID) {
    EnterFault("TURN_SENSOR_INVALID", nowMs);

    return;
  }

    if (
        g_pendingTurn ==
        TURN_LEFT
    ) {
        SetMotorTarget(
            SPEED_TURN_REVERSE,
            SPEED_TURN_FORWARD
        );
    } else {
        SetMotorTarget(
            SPEED_TURN_FORWARD,
            SPEED_TURN_REVERSE
        );
    }

  /*
   * A successful turn should leave
   * centered state and later reacquire
   * 00.
   */

  if (observation != OBS_00) {
    g_turnSawDeviation = true;

    g_turnCenterCount = 0U;

  } else if (elapsed >= TURN_MIN_MS && g_turnSawDeviation) {
    if (g_turnCenterCount < 255U) {
      ++g_turnCenterCount;
    }

    if (g_turnCenterCount >= TURN_CENTER_HOLD_SAMPLES) {
      CompleteTurn(nowMs);

      return;
    }
  }

  if (elapsed > TURN_TIMEOUT_MS) {

    if (g_turnPurpose == TURN_PURPOSE_NEW_JUNCTION) {
      /*
       * Candidate did not produce a
       * valid new branch.
       *
       * Do not consume its ordinal.
       */

      printf("TRACK,JUNCTION_REJECT,"
             "ORDINAL=%u,"
             "REASON=TURN_TIMEOUT\r\n",
             (unsigned int)g_pendingJunctionOrdinal);

      BeginUTurn(nowMs, "CANDIDATE_BRANCH_NOT_FOUND");

    } else {

      EnterFault("BACKTRACK_TURN_TIMEOUT", nowMs);
    }
  }
}

/* -------------------------------------------------------------------------- */
/* U-turn execution                                                           */
/* -------------------------------------------------------------------------- */

static void ProcessUTurnState(SensorObservation observation, uint32_t nowMs) {
  uint32_t elapsed;

  elapsed = ElapsedMs(nowMs, g_stateSinceMs);

  if (observation == OBS_INVALID) {
    EnterFault("UTURN_SENSOR_INVALID", nowMs);

    return;
  }

  /*
   * In-place U-turn.
   */

  SetMotorTarget(-SPEED_UTURN, SPEED_UTURN);

  if (observation != OBS_00) {
    g_uturnSawDeviation = true;

    g_uturnCenterCount = 0U;

  } else if (elapsed >= UTURN_MIN_MS && g_uturnSawDeviation) {
    if (g_uturnCenterCount < 255U) {
      ++g_uturnCenterCount;
    }

    if (g_uturnCenterCount >= UTURN_CENTER_HOLD_SAMPLES) {
      g_backtrackSinceMs = nowMs;

      g_lastValidTrackingMs = nowMs;

      ResetMarkerDetector();

      SetState(NAV_BACKTRACK, nowMs, "UTURN_REACQUIRED_LINE");

      printf("TRACK,UTURN_COMPLETE\r\n");

      return;
    }
  }

  if (elapsed > UTURN_TIMEOUT_MS) {
    EnterFault("UTURN_TIMEOUT", nowMs);
  }
}

/* -------------------------------------------------------------------------- */
/* Hardware initialization                                                    */
/* -------------------------------------------------------------------------- */

static int InitTrackHardware(void) {
  WifiIotUartAttribute uartAttribute = {
      .baudRate = 115200,
      .dataBits = 8,
      .stopBits = 1,
      .parity = 0,
  };

  if (GpioInit() != WIFI_IOT_SUCCESS) {
    printf("TRACK,INIT_ERROR,"
           "GPIO_INIT\r\n");

    return -1;
  }

  /*
   * UART2 TX:
   *
   * Hi3861 GPIO11
   *      ->
   * STM32 PA10 / USART1_RX
   */

  if (IoSetFunc(WIFI_IOT_IO_NAME_GPIO_11, WIFI_IOT_IO_FUNC_GPIO_11_UART2_TXD) !=
      WIFI_IOT_SUCCESS) {
    printf("TRACK,INIT_ERROR,"
           "UART2_TX_MUX\r\n");

    return -1;
  }

  /*
   * IR sensors.
   */

  if (IoSetFunc(WIFI_IOT_IO_NAME_GPIO_13, WIFI_IOT_IO_FUNC_GPIO_13_GPIO) !=
          WIFI_IOT_SUCCESS ||

      IoSetFunc(WIFI_IOT_IO_NAME_GPIO_14, WIFI_IOT_IO_FUNC_GPIO_14_GPIO) !=
          WIFI_IOT_SUCCESS) {
    printf("TRACK,INIT_ERROR,"
           "SENSOR_MUX\r\n");

    return -1;
  }

  if (GpioSetDir(TRACK_LEFT_SENSOR_GPIO, WIFI_IOT_GPIO_DIR_IN) !=
          WIFI_IOT_SUCCESS ||

      GpioSetDir(TRACK_RIGHT_SENSOR_GPIO, WIFI_IOT_GPIO_DIR_IN) !=
          WIFI_IOT_SUCCESS) {
    printf("TRACK,INIT_ERROR,"
           "SENSOR_DIR\r\n");

    return -1;
  }

  /*
   * UART2:
   *
   * 115200
   * 8 data bits
   * 1 stop bit
   * no parity
   */

  if (UartInit(TRACK_UART_INDEX, &uartAttribute, NULL) != WIFI_IOT_SUCCESS) {
    printf("TRACK,INIT_ERROR,"
           "UART2_INIT\r\n");

    return -1;
  }

  /*
   * Safety invariant:
   *
   * send STOP before autonomous task
   * starts.
   */

  if (WriteMotorTarget(0, 0) != 0) {
    printf("TRACK,INIT_ERROR,"
           "STARTUP_STOP_WRITE\r\n");

    return -1;
  }

  return 0;
}

/* -------------------------------------------------------------------------- */
/* Autonomous task                                                            */
/* -------------------------------------------------------------------------- */

static void AutonomousTrackTask(void *argument) {
  WifiIotGpioValue left = WIFI_IOT_GPIO_VALUE0;

  WifiIotGpioValue right = WIFI_IOT_GPIO_VALUE0;

  SensorObservation raw;
  SensorObservation stable;

  uint32_t nowMs;
  uint32_t startupMs;

  (void)argument;

  startupMs = NowMs();

  g_stateSinceMs = startupMs;

  g_lastMotorTxMs = startupMs;

  g_lastStatusLogMs = startupMs;

  g_lastValidTrackingMs = startupMs;

  StopMotorTarget();

  printf("TRACK,READY,"
         "AUTONOMOUS_TRACK\r\n");

  printf("TRACK,FACT,"
         "IR_LEFT=GPIO13,"
         "IR_RIGHT=GPIO14,"
         "UART2_TX=GPIO11\r\n");

  printf("TRACK,FACT,"
         "MOTOR_PROTOCOL="
         "FC_DIRL_SPEEDL_DIRR_SPEEDR_FD\r\n");

  printf("TRACK,WARN,"
         "CALIBRATION_PARAMETERS_"
         "REQUIRE_HARDWARE_TUNING\r\n");

  while (1) {

    nowMs = NowMs();

    ++g_sampleNumber;

    /*
     * Sensor acquisition.
     */

    if (ReadRawSensors(&left, &right) != 0) {
      EnterFault("SENSOR_READ_FAILURE", nowMs);

    } else {

      raw = ObservationFromRaw(left, right);

      if (raw == OBS_INVALID) {
        EnterFault("SENSOR_VALUE_ILLEGAL", nowMs);

      } else {

        stable = FilterObservation(raw);

        /*
         * Structured periodic log.
         */

        if (ElapsedMs(nowMs, g_lastStatusLogMs) >= STATUS_LOG_PERIOD_MS) {
          printf("TRACK,STATUS,"
                 "N=%lu,"
                 "STATE=%s,"
                 "RAW=%s,"
                 "STABLE=%s,"
                 "DEPTH=%u,"
                 "JUNCTIONS=%u,"
                 "L=%d,"
                 "R=%d\r\n",

                 (unsigned long)g_sampleNumber,

                 StateName(g_state),

                 ObservationName(raw),

                 ObservationName(stable),

                 (unsigned int)g_pathDepth,

                 (unsigned int)g_uniqueJunctions,

                 (int)g_targetLeft,

                 (int)g_targetRight);

          g_lastStatusLogMs = nowMs;
        }

        /*
         * Navigation FSM.
         */

        switch (g_state) {

        case NAV_WAIT_SENSOR:

          StopMotorTarget();

          if (stable != OBS_INVALID) {
            g_lastValidTrackingMs = nowMs;

            ResetMarkerDetector();

            SetState(NAV_FOLLOW_LINE, nowMs, "SENSOR_STABLE");

          } else if (ElapsedMs(nowMs, startupMs) > STARTUP_SENSOR_TIMEOUT_MS) {
            EnterFault("STARTUP_SENSOR_TIMEOUT", nowMs);
          }

          break;

        case NAV_FOLLOW_LINE:

          ProcessTrackFollowing(stable, false, nowMs);

          break;

        case NAV_CENTER_JUNCTION:

          ProcessCenterState(nowMs);

          break;

        case NAV_EXECUTE_TURN:

          ProcessTurnState(stable, nowMs);

          break;

        case NAV_U_TURN:

          ProcessUTurnState(stable, nowMs);

          break;

        case NAV_BACKTRACK:

          ProcessTrackFollowing(stable, true, nowMs);

          break;

        case NAV_FINISHED:
        case NAV_FAULT:

          /*
           * Permanent safe state.
           *
           * Sensor changes never
           * restart the vehicle.
           */

          StopMotorTarget();

          break;

        default:

          EnterFault("INVALID_NAV_STATE", nowMs);

          break;
        }
      }
    }

    /*
     * Independent motor target refresh.
     */

    if (RefreshMotorIfNeeded(nowMs) != 0 && g_state != NAV_FAULT) {
      EnterFault("UART_WRITE_FAILURE", nowMs);
    }

    usleep(SENSOR_PERIOD_MS * 1000U);
  }
}

/* -------------------------------------------------------------------------- */
/* Feature entry                                                              */
/* -------------------------------------------------------------------------- */

static void AutonomousTrackEntry(void) {
  osThreadAttr_t attribute = {0};

  if (InitTrackHardware() != 0) {
    /*
     * Best-effort STOP.
     *
     * If UART initialization did not
     * complete, STM32's own timeout is
     * still the final safety layer.
     */

    (void)WriteMotorTarget(0, 0);

    printf("TRACK,INIT_ABORT,"
           "SAFE_STOP\r\n");

    return;
  }

  attribute.name = "autonomous_track";

  attribute.stack_size = 4096U;

  attribute.priority = osPriorityNormal;

  if (osThreadNew(AutonomousTrackTask, NULL, &attribute) == NULL) {
    (void)WriteMotorTarget(0, 0);

    printf("TRACK,INIT_ERROR,"
           "TASK_CREATE\r\n");
  }
}

APP_FEATURE_INIT(AutonomousTrackEntry);