import { Calendar } from '@fullcalendar/core'
import deLocale from '@fullcalendar/core/locales/de'
import dayGridPlugin from '@fullcalendar/daygrid'
import interactionPlugin from '@fullcalendar/interaction'
import timeGridPlugin from '@fullcalendar/timegrid'

new QWebChannel(qt.webChannelTransport, (channel) => {
  const bridge = channel.objects.calendarBridge
  const calendarElement = document.getElementById('calendar')
  const pendingChanges = new Map()
  const isEditable = (event) =>
    !event.extendedProps.locked && !pendingChanges.has(event.id)
  const requestEntryChange = (info) => {
    if (!isEditable(info.event)) {
      info.revert()
      return
    }

    pendingChanges.set(info.event.id, info.revert)
    bridge.entryChangeRequested(
      Number(info.event.id),
      info.event.startStr,
      info.event.endStr,
    )
  }

  const calendar = new Calendar(calendarElement, {
    plugins: [dayGridPlugin, interactionPlugin, timeGridPlugin],
    locale: deLocale,
    firstDay: 1,
    initialView: 'timeGridWeek',
    height: '100%',
    nowIndicator: true,
    editable: true,
    eventResizableFromStart: true,
    selectable: true,
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
    eventAllow: (_dropInfo, event) => isEditable(event),
    eventDrop: requestEntryChange,
    eventResize: requestEntryChange,
    select: (info) => {
      calendar.unselect()
      bridge.timeRangeSelected(info.startStr, info.endStr)
    },
    eventClick: (info) => {
      if (isEditable(info.event)) {
        bridge.editEntryRequested(Number(info.event.id))
      }
    },
    eventDidMount: (info) => {
      info.el.addEventListener('contextmenu', (event) => {
        event.preventDefault()
        if (!pendingChanges.has(info.event.id)) {
          bridge.contextMenuRequested(
            Number(info.event.id),
            event.clientX,
            event.clientY,
          )
        }
      })
    },
    datesSet: (info) => bridge.rangeChanged(info.startStr, info.endStr),
  })

  bridge.entriesChanged.connect((entriesJson) => {
    pendingChanges.clear()
    calendar.removeAllEvents()
    calendar.addEventSource(JSON.parse(entriesJson))
  })
  bridge.entryChangeFailed.connect((id) => {
    const revert = pendingChanges.get(String(id))
    if (revert) {
      revert()
      pendingChanges.delete(String(id))
    }
  })
  bridge.resizeRequested.connect(() => calendar.updateSize())

  calendar.render()
  bridge.calendarReady()
})
