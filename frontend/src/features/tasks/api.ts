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
  detail: (entityId: number) => ['tasks', 'detail', entityId] as const,
  metadata: ['tasks', 'metadata'] as const,
}

export function getTasks() {
  return apiClient.get<Task[]>('/tasks')
}

export function getTask(entityId: number) {
  return apiClient.get<Task>(`/tasks/${entityId}`)
}

export function getTaskMetadata() {
  return apiClient.get<TaskMetadata>('/tasks/metadata')
}

export function createTask(task: CreateTask) {
  return apiClient.post<CreateTaskResponse>('/tasks', task)
}

export function updateTask(entityId: number, task: UpdateTask) {
  return apiClient.put<void>(`/tasks/${entityId}`, task)
}

export function deleteTask(entityId: number) {
  return apiClient.delete(`/tasks/${entityId}`)
}

export function useTasks() {
  return useQuery({ queryKey: taskKeys.list, queryFn: getTasks })
}

export function useTask(entityId: number) {
  return useQuery({
    queryKey: taskKeys.detail(entityId),
    queryFn: () => getTask(entityId),
  })
}

export function useTaskMetadata(enabled = true) {
  return useQuery({
    queryKey: taskKeys.metadata,
    queryFn: getTaskMetadata,
    enabled,
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
    mutationFn: ({
      entityId,
      task,
    }: {
      entityId: number
      task: UpdateTask
    }) => updateTask(entityId, task),
    onSuccess: async (_, { entityId }) => {
      await Promise.all([
        queryClient.invalidateQueries({ queryKey: taskKeys.list }),
        queryClient.invalidateQueries({
          queryKey: taskKeys.detail(entityId),
        }),
      ])
    },
  })
}

export function useDeleteTask() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: deleteTask,
    onSuccess: async (_, entityId) => {
      queryClient.removeQueries({ queryKey: taskKeys.detail(entityId) })
      await queryClient.invalidateQueries({ queryKey: taskKeys.list })
    },
  })
}
