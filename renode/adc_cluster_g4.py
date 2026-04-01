# Minimal STM32G4 ADC-cluster stub for Renode tests.
# Covers two ADC register banks in one 0x400 window (offset 0x000 and 0x100),
# plus common cluster registers around 0x300.

ADRDY = 1 << 0
EOC = 1 << 2
EOS = 1 << 3

CR_ADEN = 1 << 0
CR_ADDIS = 1 << 1
CR_ADSTART = 1 << 2
CR_ADCAL = 1 << 31

if request.IsInit:
    regs = {}
    isr = {}
    dr = {}

if 'regs' not in locals():
    regs = {}
if 'isr' not in locals():
    isr = {}
if 'dr' not in locals():
    dr = {}

def bank_base(offset):
    return offset & ~0xFF

def reg_off(offset):
    return offset & 0xFF

if request.IsWrite:
    b = bank_base(request.Offset)
    r = reg_off(request.Offset)

    # ISR: write-1-to-clear behavior
    if r == 0x00:
        prev = isr.get(b, 0)
        isr[b] = prev & (~request.Value)
        regs[request.Offset] = isr[b]

    elif r == 0x08:
        value = request.Value

        # Calibration completes immediately in this stub.
        if value & CR_ADCAL:
            value &= ~CR_ADCAL

        # ADC enable/disable reflected into ADRDY flag.
        if value & CR_ADEN:
            isr[b] = isr.get(b, 0) | ADRDY
        if value & CR_ADDIS:
            isr[b] = isr.get(b, 0) & ~ADRDY

        regs[request.Offset] = value

        # Fake one EOC/EOS event when conversion starts.
        if value & CR_ADSTART:
            isr[b] = isr.get(b, 0) | EOC | EOS

    else:
        regs[request.Offset] = request.Value

elif request.IsRead:
    b = bank_base(request.Offset)
    r = reg_off(request.Offset)

    if r == 0x00:
        request.Value = isr.get(b, ADRDY)
    elif r == 0x40:
        request.Value = dr.get(b, 0)
    else:
        request.Value = regs.get(request.Offset, 0)
