#!/usr/bin/env python
# vim: sts=4 sw=4 et

import _hal, hal, gobject
import linuxcnc

class GPin(gobject.GObject, hal.Pin):
    __gtype_name__ = 'GPin'
    __gsignals__ = {'value-changed': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, ())}

    REGISTRY = []
    UPDATE = False

    def __init__(self, *a, **kw):
        gobject.GObject.__init__(self)
        hal.Pin.__init__(self, *a, **kw)
        self._item_wrap(self._item)
        self._prev = None
        self.REGISTRY.append(self)
        self.update_start()

    def update(self):
        tmp = self.get()
        if tmp != self._prev:
            self.emit('value-changed')
        self._prev = tmp

    @classmethod
    def update_all(self):
        if not self.UPDATE:
            return
        kill = []
        for p in self.REGISTRY:
            try:
                p.update()
            except:
                kill.append(p)
                print "Error updating pin %s; Removing" % p
        for p in kill:
            self.REGISTRY.remove(p)
        return self.UPDATE

    @classmethod
    def update_start(self, timeout=100):
        if GPin.UPDATE:
            return
        GPin.UPDATE = True
        gobject.timeout_add(timeout, self.update_all)

    @classmethod
    def update_stop(self, timeout=100):
        GPin.UPDATE = False

class GComponent:
    def __init__(self, comp):
        if isinstance(comp, GComponent):
            comp = comp.comp
        self.comp = comp

    def newpin(self, *a, **kw): return GPin(_hal.component.newpin(self.comp, *a, **kw))
    def getpin(self, *a, **kw): return GPin(_hal.component.getpin(self.comp, *a, **kw))

    def exit(self, *a, **kw): return self.comp.exit(*a, **kw)

    def __getitem__(self, k): return self.comp[k]
    def __setitem__(self, k, v): self.comp[k] = v

class _GStat(gobject.GObject):
    __gsignals__ = {
        'state-estop': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, ()),
        'state-estop-reset': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, ()),
        'state-on': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, ()),
        'state-off': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, ()),
        'homed': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, (gobject.TYPE_STRING,)),
        'all-homed': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, ()),
        'not-all-homed': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, (gobject.TYPE_STRING,)),

        'mode-manual': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, ()),
        'mode-auto': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, ()),
        'mode-mdi': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, ()),

        'interp-run': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, ()),

        'interp-idle': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, ()),
        'interp-paused': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, ()),
        'interp-reading': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, ()),
        'interp-waiting': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, ()),

        'file-loaded': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, (gobject.TYPE_STRING,)),
        'reload-display': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, ()),
        'line-changed': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, (gobject.TYPE_INT,)),
        'tool-in-spindle-changed': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, (gobject.TYPE_INT,)),
        }

    STATES = { linuxcnc.STATE_ESTOP:       'state-estop'
             , linuxcnc.STATE_ESTOP_RESET: 'state-estop-reset'
             , linuxcnc.STATE_ON:          'state-on'
             , linuxcnc.STATE_OFF:         'state-off'
             }

    MODES  = { linuxcnc.MODE_MANUAL: 'mode-manual'
             , linuxcnc.MODE_AUTO:   'mode-auto'
             , linuxcnc.MODE_MDI:    'mode-mdi'
             }

    INTERP = { linuxcnc.INTERP_WAITING: 'interp-waiting'
             , linuxcnc.INTERP_READING: 'interp-reading'
             , linuxcnc.INTERP_PAUSED: 'interp-paused'
             , linuxcnc.INTERP_IDLE: 'interp-idle'
             }

    def __init__(self, stat = None):
        gobject.GObject.__init__(self)
        self.stat = stat or linuxcnc.stat()
        self.old = {}
        try:
            self.stat.poll()
            self.merge()
        except:
            pass
        gobject.timeout_add(100, self.update)

    def merge(self):
        self.old['state'] = self.stat.task_state
        self.old['mode']  = self.stat.task_mode
        self.old['interp']= self.stat.interp_state
        self.old['file']  = self.stat.file
        self.old['line']  = self.stat.motion_line
        self.old['homed'] = self.stat.homed
        self.old['tool-in-spindle'] = self.stat.tool_in_spindle

    def update(self):
        try:
            self.stat.poll()
        except:
            # Reschedule
            return True
        old = dict(self.old)
        self.merge()

        state_old = old.get('state', 0)
        state_new = self.old['state']
        if not state_old:
            if state_new > linuxcnc.STATE_ESTOP:
                self.emit('state-estop-reset')
            else:
                self.emit('state-estop')
            self.emit('state-off')
            self.emit('interp-idle')

        if state_new != state_old:
            if state_old == linuxcnc.STATE_ON and state_new < linuxcnc.STATE_ON:
                self.emit('state-off')
            self.emit(self.STATES[state_new])
            if state_new == linuxcnc.STATE_ON:
                old['mode'] = 0
                old['interp'] = 0

        mode_old = old.get('mode', 0)
        mode_new = self.old['mode']
        if mode_new != mode_old:
            self.emit(self.MODES[mode_new])

        interp_old = old.get('interp', 0)
        interp_new = self.old['interp']
        if interp_new != interp_old:
            if not interp_old or interp_old == linuxcnc.INTERP_IDLE:
                print "Emit", "interp-run"
                self.emit('interp-run')
            self.emit(self.INTERP[interp_new])

        file_old = old.get('file', None)
        file_new = self.old['file']
        if file_new != file_old:
            self.emit('file-loaded', file_new)

        line_old = old.get('line', None)
        line_new = self.old['line']
        if line_new != line_old:
            self.emit('line-changed', line_new)

        tool_old = old.get('tool-in-spindle', None)
        tool_new = self.old['tool-in-spindle']
        if tool_new != tool_old:
            self.emit('tool-in-spindle-changed', tool_new)

        # if the homed status has changed
        # check number of homed axes against number of available axes
        # if they are equal send the all-homed signal
        # else not-all-homed (with a string of unhomed joint numbers)
        # if a joint is homed send 'homed' (with a string of homed joint numbers)
        homed_old = old.get('homed', None)
        homed_new = self.old['homed']
        if homed_new != homed_old:
            axis_count = count = 0
            unhomed = homed = ""
            for i,h in enumerate(homed_new):
                if h:
                    count +=1
                    homed += str(i)
                if self.stat.axis_mask & (1<<i) == 0: continue
                axis_count += 1
                if not h:
                    unhomed += str(i)
            if count:
                self.emit('homed',homed)
            if count == axis_count:
                self.emit('all-homed')
            else:
                self.emit('not-all-homed',unhomed)

        return True

class GStat(_GStat):
    _instance = None
    def __new__(cls, *args, **kwargs):
        if not cls._instance:
            cls._instance = _GStat.__new__(cls, *args, **kwargs)
        return cls._instance
