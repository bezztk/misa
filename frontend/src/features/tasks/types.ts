export interface Task {
  item_id: number
  item_type: number
  parent_id: number | null
  created_at: string
  updated_at: string
  is_archived: boolean
  version: number
  group_id: number | null
  area_id: number | null
  state_id: number
  name: string
  description: string | null
  completed_at: string | null
}

export interface CreateTask {
  name: string
  group_id: number | null
  area_id: number | null
  description: string | null
}

export interface UpdateTask {
  name?: string
  state_id?: number | null
  group_id?: number | null
  description?: string | null
}

export interface CreateTaskResponse {
  id: number
}

export interface TaskGroup {
  id: number
  name: string
}

export interface TaskState {
  id: number
  name: string
}

export interface TaskMetadata {
  groups: TaskGroup[]
  states: TaskState[]
}

