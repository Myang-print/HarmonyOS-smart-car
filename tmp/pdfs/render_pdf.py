import fitz
from PIL import Image, ImageDraw

document = fitz.open(r"C:\Users\Thinkpad\Downloads\Schematic_QST-鸿蒙小车.pdf")
print(f"pages={len(document)}")
for index, page in enumerate(document):
    pixmap = page.get_pixmap(matrix=fitz.Matrix(3, 3), alpha=False)
    pixmap.save(
        rf"D:\_GitHub\30_MyProjects\HarmonyOS-smart-car\tmp\pdfs\qst-schematic-{index + 1}.png"
    )

thumbs = []
for index in range(len(document)):
    image = Image.open(
        rf"D:\_GitHub\30_MyProjects\HarmonyOS-smart-car\tmp\pdfs\qst-schematic-{index + 1}.png"
    )
    image.thumbnail((640, 460))
    thumbs.append(image.copy())

sheet = Image.new("RGB", (1280, 460 * 6), "white")
draw = ImageDraw.Draw(sheet)
for index, image in enumerate(thumbs):
    x = (index % 2) * 640
    y = (index // 2) * 460
    sheet.paste(image, (x, y))
    draw.text((x + 8, y + 8), f"Page {index + 1}", fill="red")
sheet.save(r"D:\_GitHub\30_MyProjects\HarmonyOS-smart-car\tmp\pdfs\qst-schematic-contact.png")
