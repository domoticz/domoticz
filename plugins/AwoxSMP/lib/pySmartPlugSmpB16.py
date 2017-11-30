import binascii
import struct
from bluepy import btle

START_OF_MESSAGE = b'\x0f'
END_OF_MESSAGE = b'\xff\xff'

class SmartPlug(btle.Peripheral):
    def __init__(self, addr):
        btle.Peripheral.__init__(self, addr)
        self.delegate = NotificationDelegate()
        self.setDelegate(self.delegate)
        self.plug_svc = self.getServiceByUUID('0000fff0-0000-1000-8000-00805f9b34fb')
        self.plug_cmd_ch = self.plug_svc.getCharacteristics('0000fff3-0000-1000-8000-00805f9b34fb')[0]

    def on(self):
        self.delegate.chg_is_ok = False
        self.plug_cmd_ch.write(self.get_buffer(binascii.unhexlify('0300010000')))
        self.wait_data(0.5)
        return self.delegate.chg_is_ok

    def off(self):
        self.delegate.chg_is_ok = False
        self.plug_cmd_ch.write(self.get_buffer(binascii.unhexlify('0300000000')))
        self.wait_data(0.5)
        return self.delegate.chg_is_ok

    def status_request(self):
        self.plug_cmd_ch.write(self.get_buffer(binascii.unhexlify('04000000')))
        self.wait_data(2.0)
        return self.delegate.state, self.delegate.power

    def program_request(self):
        self.plug_cmd_ch.write(self.get_buffer(binascii.unhexlify('07000000')))
        self.wait_data(2.0)
        return self.delegate.programs

    def calculate_checksum(self, message):
        return (sum(bytearray(message)) + 1) & 0xff

    def get_buffer(self, message):
        return START_OF_MESSAGE + struct.pack("b",len(message) + 1) + message + struct.pack("b",self.calculate_checksum(message)) + END_OF_MESSAGE 

    def wait_data(self, timeout):
        self.delegate.need_data = True
        while self.delegate.need_data and self.waitForNotifications(timeout):
            pass

class NotificationDelegate(btle.DefaultDelegate):
    def __init__(self):
        btle.DefaultDelegate.__init__(self)
        self.state = False
        self.power = 0
        self.chg_is_ok = False
        self.programs = []
        self._buffer = b''
        self.need_data = True

    def handleNotification(self, cHandle, data):
        #not sure 0x0f indicate begin of buffer but
        if data[:1] == START_OF_MESSAGE:
            self._buffer = data
        else:
            self._buffer = self._buffer + data
        if self._buffer[-2:] == END_OF_MESSAGE:
            self.handle_data(self._buffer)
            self._buffer = b''
            self.need_data = False 

    def handle_data(self, bytes_data):
        # it's a state change confirm notification ?
        if bytes_data[0:3] == b'\x0f\x04\x03':
            self.chg_is_ok = True
        # it's a state/power notification ?
        if bytes_data[0:3] == b'\x0f\x0f\x04':
            (state, dummy, power) = struct.unpack_from(">?BI", bytes_data, offset=4)
            self.state = state
            self.power = power / 1000
        # it's a 0x0a notif ?
        if bytes_data[0:3] == b'\x0f\x33\x0a':
            print ("0A notif %s" % bytes_data)
        # it's a programs notif ?
        if bytes_data[0:3] == b'\x0f\x71\x07' :
            program_offset = 4
            self.programs = []
            while program_offset + 21 < len(bytes_data):
                (present, name, flags, start_hour, start_minute, end_hour, end_minute) = struct.unpack_from(">?16sbbbbb", bytes_data, program_offset)
                #TODO interpret flags (day of program ?)
                if present:
                    self.programs.append({ "name" : name.decode('iso-8859-1').strip('\0'), "flags":flags, "start":"{0:02d}:{1:02d}".format(start_hour, start_minute), "end":"{0:02d}:{1:02d}".format(end_hour, end_minute)})
                program_offset += 22

# SmartPlugSmpB16 usage sample: cycle power then log plug state and power level to terminal
if __name__ == '__main__':
    import time

    # connect to the plug with bluetooth address
    plug = SmartPlug('98:7B:F3:34:78:52')

    # cycle power
    plug.off()
    time.sleep(2.0)
    plug.on()

    # display state and power level
    while True:
        (state, power) = plug.status_request()
        print('plug state = %s' % ('on' if state else 'off'))
        print('plug power = %d W' % power)
        time.sleep(2.0)
