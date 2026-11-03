import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query'
import { apiClient } from '../../api/client'
import type {
  CreateTask,
  CreateTaskResponse,
  Task,
  TaskMetadata,
  UpdateTask,
} from './types'

const taskKeys = {
  list: ['tasks', 'list'] as const,
  detail: (id: number) => ['tasks', 'detail', id] as const,
  metadata: ['tasks', 'metadata'] as const,
}

export function getTasks() {
  return apiClient.get<Task[]>('/tasks')
}

export function getTask(id: number) {
  return apiClient.get<Task>(`/tasks/${id}`)
}

export function getTaskMetadata() {
  return apiClient.get<TaskMetadata>('/tasks/metadata')
}

export function createTask(task: CreateTask) {
  return apiClient.post<CreateTaskResponse>('/tasks', task)
}

export function updateTask(id: number, task: UpdateTask) {
  return apiClient.patch<void>(`/tasks/${id}`, task)
}

export function deleteTask(id: number) {
  return apiClient.delete(`/tasks/${id}`)
}

export function useTasks() {
  return useQuery({ queryKey: taskKeys.list, queryFn: getTasks })
}

export function useTask(id: number) {
  return useQuery({
    queryKey: taskKeys.detail(id),
    queryFn: () => getTask(id),
  })
}

export function useTaskMetadata() {
  return useQuery({
    queryKey: taskKeys.metadata,
    queryFn: getTaskMetadata,
  })
}

export function useCreateTask() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: createTask,
    onSuccess: () =>
      queryClient.invalidateQueries({ queryKey: taskKeys.list }),
  })
}

export function useUpdateTask() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: ({ id, task }: { id: number; task: UpdateTask }) =>
      updateTask(id, task),
    onSuccess: async (_, { id }) => {
      await Promise.all([
        queryClient.invalidateQueries({ queryKey: taskKeys.list }),
        queryClient.invalidateQueries({ queryKey: taskKeys.detail(id) }),
      ])
    },
  })
}

export function useDeleteTask() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: deleteTask,
    onSuccess: async (_, id) => {
      queryClient.removeQueries({ queryKey: taskKeys.detail(id) })
      await queryClient.invalidateQueries({ queryKey: taskKeys.list })
    },
  })
}
