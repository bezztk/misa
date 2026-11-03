import { createFileRoute } from '@tanstack/react-router'
import { TasksTable } from '../features/tasks/TasksTable'

export const Route = createFileRoute('/tasks')({
  component: TasksTable,
})
