require 'image'
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
    compressed = t.libcompress.compress(t)
    print("Normal size: " .. t:storage():size())
    print("Compressed size: " .. compressed:size())
    --print("Decompressing")
    --decompressed = t.libcompress.decompress(compressed)
    --print("Normal size = " .. t:storage():size() .. " in items, not bytes")
    --print("Compressed size = " .. compressed:storage():size() .. "in bytes")
    --delta = t:clone():mul(-1):add(decompressed)
    --print("After round tripping, difference is " .. delta:norm(2))
end
