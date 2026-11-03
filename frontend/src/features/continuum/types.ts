export interface DevelopmentArea {
  id: number
  name: string
  description: string | null
}

export interface CreateDevelopmentArea {
  name: string
  description: string | null
}

export interface UpdateDevelopmentArea {
  name?: string
  description?: string | null
}

export interface CreateDevelopmentAreaResponse {
  id: number
}

export interface DevelopmentCategory {
  item_id: number
  area_id: number
  name: string
}

export interface CreateDevelopmentCategory {
  area_id: number
  name: string
}

export interface UpdateDevelopmentCategory {
  name?: string
}

export interface CreateDevelopmentCategoryResponse {
  id: number
}

export interface Skill {
  item_id: number
  area_id: number
  state_id: number
  name: string
  description: string | null
}

export interface CreateSkill {
  area_id: number
  state_id: number | null
  name: string
  description: string | null
}

export interface UpdateSkill {
  state_id?: number | null
  name?: string
  description?: string | null
}

export interface CreateSkillResponse {
  id: number
}

export interface SkillState {
  id: number
  name: string
}

export interface SkillMetadata {
  states: SkillState[]
}

export interface Goal {
  item_id: number
  area_id: number
  name: string
  description: string | null
  is_completed: boolean
  completed_at: string | null
  due_at: string | null
}

export interface CreateGoal {
  area_id: number
  name: string
  description: string | null
  due_at: string | null
}

export interface UpdateGoal {
  name?: string
  description?: string | null
  is_completed?: boolean | null
  due_at?: string | null
  clear_due_at?: boolean | null
}

export interface CreateGoalResponse {
  id: number
}

export interface ItemHierarchy {
  child_id: number
  parent_id: number
}

export interface AllowedTypeRelation {
  parent_type: string
  child_type: string
}

export interface UpdateItemParent {
  parent_id: number | null
}

export interface UpdateItemArchive {
  is_archived: boolean
}
