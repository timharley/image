require 'image'
require 'math'

lena3d = image.load("lena.png", nil, "byte")
lena2d = lena3d:transpose(1,2):clone():transpose(2,1):select(1, 2)
lena4d = lena3d:repeatTensor(3,1,1,1)
local tensors = {lena3d, lena2d, lena4d} --, torch.FloatTensor(), torch.DoubleTensor()}

for i, t in ipairs(tensors) do
    print("--------TESTING--------")
    print("Image dims = ")
    print(t:size())
    print("Image strides = ")
    print(t:stride())
    print("Compressing:")
    compressed = image.compress(t)
    print("Normal size: " .. t:storage():size())
    print("Compressed size: " .. compressed.data:size())
    -- print(compressed)
    decompressed = compressed:decompress()
    print("Decompressed type: " .. decompressed:type())
    print("Decompressed size: " )
    print(decompressed:size())
    local sum = 0
    max = t:double():mul(-1):add(decompressed:double()):abs():max()
    delta = t:clone():mul(-1):add(decompressed):apply( function(x) sum = sum + math.abs(x) end )
    print("After round tripping, difference is " .. sum .. " max is " .. max)
end
