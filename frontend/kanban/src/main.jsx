import { useEffect, useMemo, useState } from 'react'
import { createRoot } from 'react-dom/client'
import { Kanban, WillowDark } from '@svar-ui/react-kanban'
import '@svar-ui/react-kanban/all.css'
import './style.css'

function TaskCard({ card }) {
  const openContextMenu = (event) => {
    event.preventDefault()
    window.taskBridge.contextMenuRequested(
      Number(card.id),
      event.clientX,
      event.clientY,
    )
  }

  return (
    <div
      className="task-card-content"
      onDoubleClick={() => window.taskBridge.editTaskRequested(Number(card.id))}
      onContextMenu={openContextMenu}
    >
      <span
        className="priority"
        style={{ backgroundColor: card.priorityColor }}
      />
      <span className="task-name">{card.label}</span>
      {card.commentCount > 0 && (
        <span className="comment-count" title="Kommentare">
          {card.commentCount}
        </span>
      )}
    </div>
  )
}

function TaskBoard({ bridge }) {
  const [data, setData] = useState({
    tasks: [],
    states: [],
    groups: [],
    domainNodes: [],
  })
  const [search, setSearch] = useState('')
  const [filterType, setFilterType] = useState('group')
  const [filterId, setFilterId] = useState('all')

  useEffect(() => {
    window.taskBridge = bridge
    bridge.boardChanged.connect((json) => setData(JSON.parse(json)))
    bridge.kanbanReady()
  }, [bridge])

  const cards = useMemo(() => {
    const term = search.trim().toLocaleLowerCase('de')
    return data.tasks.filter((task) => {
      const matchesSearch = !term || task.label.toLocaleLowerCase('de').includes(term)
      const matchesFilter =
        filterId === 'all' ||
        (filterType === 'group'
          ? task.groupId === Number(filterId)
          : task.domainNodeId === Number(filterId))
      return matchesSearch && matchesFilter
    })
  }, [data.tasks, search, filterType, filterId])

  const options = filterType === 'group' ? data.groups : data.domainNodes

  return (
    <WillowDark>
      <main>
        <div className="filters">
          <input
            type="search"
            value={search}
            onChange={(event) => setSearch(event.target.value)}
            placeholder="Suche..."
            aria-label="Aufgaben suchen"
          />
          <select
            value={filterType}
            onChange={(event) => {
              setFilterType(event.target.value)
              setFilterId('all')
            }}
            aria-label="Filtertyp"
          >
            <option value="group">Gruppe</option>
            <option value="domainNode">Wissensknoten</option>
          </select>
          <select
            value={filterId}
            onChange={(event) => setFilterId(event.target.value)}
            aria-label="Filterauswahl"
          >
            <option value="all">Alle</option>
            {options.map((option) => (
              <option key={option.id} value={option.id}>
                {option.name}
              </option>
            ))}
          </select>
        </div>
        <div className="board">
          <Kanban
            cards={cards}
            columns={data.states}
            cardContent={TaskCard}
            card={{ menu: false }}
            onMoveCard={({ id, column }) => {
              const taskId = Number(id)
              const stateId = Number(column)
              const task = data.tasks.find((item) => item.id === taskId)
              if (task && column != null && task.column !== stateId) {
                setData((current) => ({
                  ...current,
                  tasks: current.tasks.map((item) =>
                    item.id === taskId ? { ...item, column: stateId } : item,
                  ),
                }))
                bridge.taskStateChangeRequested(taskId, stateId)
              }
            }}
          />
        </div>
      </main>
    </WillowDark>
  )
}

new QWebChannel(qt.webChannelTransport, (channel) => {
  createRoot(document.getElementById('root')).render(
    <TaskBoard bridge={channel.objects.taskBridge} />,
  )
})
