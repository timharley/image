require 'image'
require 'math'
require 'libpng'
require 'libcompress'


local function test_tensortype(tensortype)
    local a = image.load("lena.png", tensortype)
    local b = image.compress(a)
    local c = image.decompress(b)
end

local tensors = {torch.ByteTensor()} --, torch.FloatTensor(), torch.DoubleTensor()}

for i, t in ipairs(tensors) do
    t = t.libpng.load("lena.png")
    print("loaded into: ", t:type())
    print("Image dims = ")
    print(t:size())
    print("Compressing:")
    compressed = image.compress(t)
    print("Normal size: " .. t:storage():size())
    print("Compressed size: " .. compressed.data:size())
    print(compressed)
    decompressed = image.decompress(compressed)
    print("Decompressed type: " .. decompressed:type())
    print("Decompressed size: " )
    print(decompressed:size())

    --print("Decompressing")
    --decompressed = t.libcompress.decompress(compressed)
    --print("Normal size = " .. t:storage():size() .. " in items, not bytes")
    --print("Compressed size = " .. compressed:storage():size() .. "in bytes")
    local sum = 0
    delta = t:clone():mul(-1):add(decompressed):apply( function(x) sum = sum + math.abs(x) end )
    print("After round tripping, difference is " .. sum)
end
