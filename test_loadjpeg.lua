require 'image'
require 'totem'

local tester = totem.Tester()
local tests = {}

function tests.loadJpgFromFile()
    x = image.loadJPG(paths.thisfile('./lena.jpg'))
    y = image.lena()
    tester:assertTensorEq(x, y, 1e-16, 'Jpg loaded from file is corrupted')
end

function tests.loadJpgFromTensor()
    local lena_size = 303179
    local f = torch.DiskFile(paths.thisfile('./lena.jpg'), 'r')
    f:binary()
    lena_compressed = torch.ByteTensor(f:readByte(lena_size))
    f:close()
    x = image.loadJPG(lena_compressed)
    y = image.lena()
    tester:assertTensorEq(x, y, 1e-16, 'Jpg loaded from ByteTensor is corrupted')
end

tester:add(tests)
return tester:run()
