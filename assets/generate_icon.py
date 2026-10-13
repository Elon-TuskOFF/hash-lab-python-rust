"""Generate an original Hash Lab icon in ICO format for Windows builds."""
from PIL import Image, ImageDraw

S = 256
scale = 3
im = Image.new("RGBA", (S * scale, S * scale), (0, 0, 0, 0))
d = ImageDraw.Draw(im)
def box(coords, fill, radius=0, outline=None, width=1):
    b=tuple(int(x*scale) for x in coords)
    d.rounded_rectangle(b,radius=radius*scale,fill=fill,outline=outline,width=width*scale)
def line(points, fill, width):
    d.line([(int(x*scale),int(y*scale)) for x,y in points], fill=fill,width=width*scale,joint="curve")

# Minimal distinctive monogram: angled brackets surrounding a secure hash mark.
box((10,10,246,246), "#12151B", 49, "#3A4356", 3)
box((27,27,229,229), "#1D222C", 38)
line([(91,84),(49,128),(91,172)],"#91AAFF",16)
line([(165,84),(207,128),(165,172)],"#91AAFF",16)
line([(125,80),(108,176)],"#80E6C4",13)
line([(159,80),(142,176)],"#80E6C4",13)
line([(105,115),(165,115)],"#80E6C4",11)
line([(98,149),(158,149)],"#80E6C4",11)
im = im.resize((256,256), Image.Resampling.LANCZOS)
im.save("assets/hashlab.ico",format="ICO",sizes=[(16,16),(24,24),(32,32),(48,48),(64,64),(128,128),(256,256)])
