if request.IsInit:
    regs = {}
    state = {'hsion': 1, 'pllon': 0, 'sw': 0, 'lsion': 0}

if 'regs' not in locals():
    regs = {}
if 'state' not in locals():
    state = {'hsion': 1, 'pllon': 0, 'sw': 0, 'lsion': 0}

if request.IsWrite:
    regs[request.Offset] = request.Value

    if request.Offset == 0x0:
        state['hsion'] = (request.Value >> 8) & 0x1
        state['pllon'] = (request.Value >> 24) & 0x1
    elif request.Offset == 0x8:
        state['sw'] = request.Value & 0x3
    elif request.Offset == 0x60:
        state['lsion'] = request.Value & 0x1

elif request.IsRead:
    if request.Offset == 0x0:
        base = regs.get(0x0, 0)
        request.Value = (base & ~((1 << 8) | (1 << 10) | (1 << 24) | (1 << 25))) \
            | (state['hsion'] << 8) | (state['hsion'] << 10) \
            | (state['pllon'] << 24) | (state['pllon'] << 25)
    elif request.Offset == 0x8:
        base = regs.get(0x8, 0)
        request.Value = (base & ~(0x3 << 2)) | ((state['sw'] & 0x3) << 2)
    elif request.Offset == 0x60:
        base = regs.get(0x60, 0)
        request.Value = (base & ~((1 << 0) | (1 << 1))) \
            | (state['lsion'] << 0) | (state['lsion'] << 1)
    else:
        request.Value = regs.get(request.Offset, 0)
