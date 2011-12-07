require "ratchet"
require "ratchet.socketpad"

function ctx1(where)
    local rec = ratchet.socket.prepare_uri(where)
    local socket = ratchet.socket.new(rec.family, rec.socktype, rec.protocol)
    socket.SO_REUSEADDR = true
    socket:bind(rec.addr)
    socket:listen()

    kernel:attach(ctx3, "tcp://localhost:10025")

    local client = socket:accept()
    kernel:attach(ctx2, client)
end

function ctx2(socket)
    local pad = ratchet.socketpad.new(socket)

    -- Portion being tested.
    --
    local data = socket:recv()
    assert('line one\r\nline two\r\n' == data)
    socket:send('break')

    local data1 = pad:recv(3)
    assert(3 == #pad:peek())
    local data2 = pad:recv(3)
    assert(0 == #pad:peek())
    assert('abc' == data1)
    assert('123' == data2)
    socket:send('break')

    local data1 = pad:recv('\r\n')
    assert(8 == #pad:peek())
    local data2 = pad:recv('\r\n')
    assert(0 == #pad:peek())
    assert('line 1\r\n' == data1)
    assert('line 2\r\n' == data2)
    socket:send('break')
end

function ctx3(where)
    local rec = ratchet.socket.prepare_uri(where)
    local socket = ratchet.socket.new(rec.family, rec.socktype, rec.protocol)
    socket:connect(rec.addr)

    local pad = ratchet.socketpad.new(socket)

    -- Portion being tested.
    --
    pad:send('line one\r\n', true)
    pad:send('line two\r\n')
    assert('break' == socket:recv())

    pad:send('abc123')
    assert('break' == socket:recv())

    pad:send('line 1\r\nline 2\r\n')
    assert('break' == socket:recv())
end

kernel = ratchet.new()
kernel:attach(ctx1, "tcp://localhost:10025")
kernel:loop()

-- vim:foldmethod=marker:sw=4:ts=4:sts=4:et:
