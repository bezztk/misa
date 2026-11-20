import { Calendar } from '@fullcalendar/core'
import deLocale from '@fullcalendar/core/locales/de'
import dayGridPlugin from '@fullcalendar/daygrid'
import timeGridPlugin from '@fullcalendar/timegrid'

new QWebChannel(qt.webChannelTransport, (channel) => {
  const bridge = channel.objects.calendarBridge
  const calendarElement = document.getElementById('calendar')
  const calendar = new Calendar(calendarElement, {
    plugins: [dayGridPlugin, timeGridPlugin],
    locale: deLocale,
    firstDay: 1,
    initialView: 'timeGridWeek',
    height: '100%',
    nowIndicator: true,
    editable: false,
    headerToolbar: {
      left: 'prev,next today',
      center: 'title',
      right: 'refresh,newEntry timeGridDay,timeGridWeek,dayGridMonth',
    },
    customButtons: {
      refresh: {
        text: 'Aktualisieren',
        click: () => bridge.refreshRequested(),
      },
      newEntry: {
        text: 'Neuer Eintrag',
        click: () => bridge.createEntryRequested(),
      },
    },
    buttonText: {
      today: 'Heute',
      day: 'Tag',
      week: 'Woche',
      month: 'Monat',
    },
    slotLabelFormat: {
      hour: '2-digit',
      minute: '2-digit',
      hour12: false,
    },
    eventTimeFormat: {
      hour: '2-digit',
      minute: '2-digit',
      hour12: false,
    },
    datesSet: (info) => bridge.rangeChanged(info.startStr, info.endStr),
  })

  bridge.entriesChanged.connect((entriesJson) => {
    calendar.removeAllEvents()
    calendar.addEventSource(JSON.parse(entriesJson))
  })
  bridge.resizeRequested.connect(() => calendar.updateSize())

  calendar.render()
  bridge.calendarReady()
})
