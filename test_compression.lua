require 'image'
require 'math'

local lena3d = image.load("lena.png", nil, "byte") -- A standard, 3-channel image tensor
local lena2d_a = lena3d:transpose(1,2):clone():transpose(2,1):select(1, 2) -- Twiddle things so that they aren't contiguous
local lena2d_b = lena3d:transpose(1,3):clone():transpose(3,1):select(1, 2) -- Twiddle things so that they aren't contiguous
local lena3d_b = lena2d_a:repeatTensor(4,1,1) -- Twiddle things so that they aren't contiguous
local lena4d = lena3d:repeatTensor(3,1,1,1) -- A 4D tensor

local function num_pixels(tensor)
    local prod = 1
    for i=1, tensor:size():size() do
        prod = prod * tensor:size()[i]
    end
    return prod
end

local tensors = {lena3d, lena3d_b, lena2d_a, lena2d_b, lena4d} 

for i, t in ipairs(tensors) do
    local t0 = sys.clock()
    local compressed = image.compress(t)
    local t1 = sys.clock()
    local decompressed = compressed:decompress()
    local t2 = sys.clock()
    print("--------TESTING--------")
    print("Normal size: " .. num_pixels(t) .. " pixels.")
    print("Compressed size: " .. compressed.data:size())
    print("Compression ratio: " .. (num_pixels(t) / compressed.data:size()))
    print("Compression rate (Mpixel/sec): " .. num_pixels(t) * 10^-6 / (t1-t0))
    print("Decompression rate ( Mpixel/sec): " .. num_pixels(t) * 10^-6 / (t2-t1))
    local sum = 0
    local delta = t:clone():mul(-1):add(decompressed):apply( function(x) sum = sum + math.abs(x) end )
    print("After round tripping, difference is " .. sum)
    assert(sum == 0, "Decompressed image does not match original image!")
end
