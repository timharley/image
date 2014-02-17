require 'image'


a = image.load("lena.png")
b = image.compress(a)
c = image.decompress(b)
