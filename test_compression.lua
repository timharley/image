require 'image'
require 'math'

lena3d = image.load("lena.png", nil, "byte") -- A standard, 3-channel image tensor
lena2d_a = lena3d:transpose(1,2):clone():transpose(2,1):select(1, 2) -- Twiddle things so that they aren't contiguous
lena2d_b = lena3d:transpose(1,3):clone():transpose(3,1):select(1, 2) -- Twiddle things so that they aren't contiguous
lena3d_b = lena2d_a:repeatTensor(4,1,1)
lena4d = lena3d:repeatTensor(3,1,1,1) -- A 4D tensor

local function num_pixels(tensor)
    local prod = 1
    for i=1, tensor:size():size() do
        prod = prod * tensor:size()[i]
    end
    return prod
end

local tensors = {lena3d, lena3d_b, lena2d_a, lena2d_b, lena4d} 

for i, t in ipairs(tensors) do
    t = t:contiguous()
    t0 = sys.clock()
    compressed = image.compress(t)
    t1 = sys.clock()
    decompressed = compressed:decompress()
    t2 = sys.clock()
    print("--------TESTING--------")
    -- print("Image dims = ")
    -- print(t:size())
    -- print("Image strides = ")
    -- print(t:stride())
    -- print("Compressing:")
    print("Normal size: " .. num_pixels(t) .. " pixels.")
    print("Compressed size: " .. compressed.data:size())
    print("Compression ratio: " .. (num_pixels(t) / compressed.data:size()))
    print("Compression rate (Mpixel/sec): " .. num_pixels(t) * 10^-6 / (t1-t0))
    print("Decompression rate ( Mpixel/sec): " .. num_pixels(t) * 10^-6 / (t2-t1))
    local sum = 0
    max = t:double():mul(-1):add(decompressed:double()):abs():max()
    delta = t:clone():mul(-1):add(decompressed):apply( function(x) sum = sum + math.abs(x) end )
    print("After round tripping, difference is " .. sum .. " max is " .. max)
end
