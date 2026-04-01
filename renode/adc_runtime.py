#
# Shared Renode helper for Peleng ADC feeds.
# Works in monitor context (IronPython 2).
#

import json
import System
from System import Array, Byte
from Antmicro.Renode.Time import TimeInterval


peleng_adc_runtime = None


def _to_bool(value):
    return str(value).strip().lower() in ("1", "true", "yes", "on")


class _PelengAdcRuntime(object):
    def __init__(self, manifest_path, loop_frames):
        if manifest_path.startswith("@"):
            manifest_path = manifest_path[1:]

        self.manifest_path = manifest_path
        self.machine = monitor.Machine
        self.bus = self.machine.SystemBus
        self.cpus = list(self.bus.GetCPUs())
        self.cpu = self.cpus[0] if self.cpus else None

        manifest_text = System.IO.File.ReadAllText(manifest_path)
        manifest = json.loads(manifest_text)

        self.frame_bytes = int(manifest["frame_bytes"])
        self.frame_count = int(manifest["frame_count"])
        self.half_period_s = float(manifest["half_period_s"])
        self.startup_delay_s = float(manifest["startup_delay_s"])
        self.loop_frames = loop_frames or bool(manifest.get("loop_frames", False))
        self.frames = []
        for frame_paths in manifest["frames"]:
            self.frames.append(
                [System.IO.File.ReadAllBytes(path) for path in frame_paths]
            )

        self.thread = None
        self.done = False
        self.addresses_ready = False
        self.frame_index = 0
        self.next_half = True

    def should_stop(self):
        return self.done

    def _read_symbol_value(self, symbol):
        if self.cpu is None:
            return None

        found, addresses = self.bus.TryGetAllSymbolAddresses(symbol, context=self.cpu)
        if not found:
            return None

        addresses = list(addresses)
        if len(addresses) == 0:
            return None

        return int(self.bus.ReadDoubleWord(addresses[0]))

    def resolve_guest_addresses(self):
        if self.addresses_ready:
            return True

        symbols = [
            "g_renode_adc_buffer0_addr",
            "g_renode_adc_buffer1_addr",
            "g_renode_adc_buffer2_addr",
            "g_renode_adc_buffer3_addr",
            "g_renode_adc_half_flag_addr",
            "g_renode_adc_full_flag_addr",
            "g_renode_delay12_addr",
            "g_renode_delay13_addr",
            "g_renode_delay14_addr",
            "g_renode_delay_valid_addr",
        ]

        values = [self._read_symbol_value(symbol) for symbol in symbols]
        if any(value is None or value == 0 for value in values):
            return False

        self.buffer_addrs = values[0:4]
        self.half_flag_addr = values[4]
        self.full_flag_addr = values[5]
        self.delay12_addr = values[6]
        self.delay13_addr = values[7]
        self.delay14_addr = values[8]
        self.delay_valid_addr = values[9]
        self.addresses_ready = True
        return True

    def _flags_pending(self):
        half_pending = int(self.bus.ReadByte(self.half_flag_addr)) != 0
        full_pending = int(self.bus.ReadByte(self.full_flag_addr)) != 0
        return half_pending or full_pending

    def _write_flag(self, address):
        self.bus.WriteBytes(Array[Byte]([1]), address)

    def feed_once(self):
        if self.done or self.frame_count == 0:
            return

        if not self.resolve_guest_addresses():
            return

        if self._flags_pending():
            return

        if self.frame_index >= self.frame_count:
            if self.loop_frames:
                self.frame_index = 0
            else:
                self.done = True
                return

        offset = 0 if self.next_half else self.frame_bytes
        flag_addr = self.half_flag_addr if self.next_half else self.full_flag_addr
        frame = self.frames[self.frame_index]

        for channel_index in range(4):
            self.bus.WriteBytes(frame[channel_index], self.buffer_addrs[channel_index] + offset)

        self._write_flag(flag_addr)
        self.frame_index += 1
        self.next_half = not self.next_half

    def start(self):
        if self.thread is not None:
            return

        interval = TimeInterval.FromSeconds(self.half_period_s)
        self.thread = self.machine.ObtainManagedThread(
            self.feed_once,
            interval,
            "peleng-adc-feed",
            self.bus,
            self.should_stop,
        )
        self.thread.StartDelayed(TimeInterval.FromSeconds(self.startup_delay_s))

    def stop(self):
        if self.thread is None:
            return
        self.thread.Stop()
        self.thread = None

    def print_results(self):
        if not self.resolve_guest_addresses():
            print "PELENG_RESULT_BEGIN"
            print "ERROR"
            print "PELENG_RESULT_END"
            return

        print "PELENG_RESULT_BEGIN"
        print "0x%08X" % int(self.bus.ReadDoubleWord(self.delay12_addr))
        print "0x%08X" % int(self.bus.ReadDoubleWord(self.delay13_addr))
        print "0x%08X" % int(self.bus.ReadDoubleWord(self.delay14_addr))
        print "0x%02X" % int(self.bus.ReadByte(self.delay_valid_addr))
        print "PELENG_RESULT_END"


def mc_peleng_adc_setup(manifest_path, loop_frames = "false"):
    global peleng_adc_runtime
    if peleng_adc_runtime is not None:
        peleng_adc_runtime.stop()

    peleng_adc_runtime = _PelengAdcRuntime(manifest_path, _to_bool(loop_frames))


def mc_peleng_adc_start():
    if peleng_adc_runtime is None:
        raise Exception("peleng_adc_setup must be called before peleng_adc_start")
    peleng_adc_runtime.start()


def mc_peleng_adc_stop():
    if peleng_adc_runtime is not None:
        peleng_adc_runtime.stop()


def mc_peleng_adc_print_results():
    if peleng_adc_runtime is None:
        raise Exception("peleng_adc_setup must be called before peleng_adc_print_results")
    peleng_adc_runtime.print_results()
