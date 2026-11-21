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
      <span className="task-text">
        <span className="task-name">{card.label}</span>
        {card.assignment && (
          <span className="task-assignment">{card.assignment}</span>
        )}
      </span>
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
  const [groupId, setGroupId] = useState('all')
  const [domainNodeId, setDomainNodeId] = useState('all')

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
        (groupId === 'all' && domainNodeId === 'all') ||
        (groupId !== 'all' && task.groupId === Number(groupId)) ||
        (domainNodeId !== 'all' && task.domainNodeId === Number(domainNodeId))
      return matchesSearch && matchesFilter
    })
  }, [data.tasks, search, groupId, domainNodeId])

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
            value={groupId}
            onChange={(event) => {
              setGroupId(event.target.value)
              setDomainNodeId('all')
            }}
            aria-label="Gruppe"
          >
            <option value="all">Alle Gruppen</option>
            {data.groups.map((group) => (
              <option key={group.id} value={group.id}>
                {group.name}
              </option>
            ))}
          </select>
          <select
            value={domainNodeId}
            onChange={(event) => {
              setDomainNodeId(event.target.value)
              setGroupId('all')
            }}
            aria-label="Wissensknoten"
          >
            <option value="all">Alle Wissensknoten</option>
            {data.domainNodes.map((node) => (
              <option key={node.id} value={node.id}>
                {node.name}
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
            render={{ fixedColumnWidth: false }}
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
