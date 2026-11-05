export interface Task {
  entity_id: number
  created_at: string
  updated_at: string
  version: number
  group_id: number
  name: string
  description: string | null
  state_id: number
  priority_id: number
  due_at: string | null
  completed_at: string | null
}

export interface CreateTask {
  group_id: number
  name: string
  description: string | null
  priority_id: number
  due_at: string | null
}

export interface UpdateTask {
  group_id: number
  name: string
  description: string | null
  state_id: number
  priority_id: number
  due_at: string | null
}

export interface CreateTaskResponse {
  entity_id: number
}

export interface TaskGroup {
  id: number
  name: string
}

export interface TaskState {
  id: number
  name: string
}

export interface TaskPriority {
  id: number
  name: string
}

export interface TaskMetadata {
  groups: TaskGroup[]
  states: TaskState[]
  priorities: TaskPriority[]
}

