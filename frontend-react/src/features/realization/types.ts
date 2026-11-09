export interface CreateTopic {
  parent_topic_entity_id: number | null
  development_area_id: number | null
  name: string
}

export interface UpdateTopic extends CreateTopic {
  understanding_level_id: number
}

export interface CreateTopicResponse {
  entity_id: number
}

export interface Topic {
  entity_id: number
  created_at: string
  updated_at: string
  version: number
  parent_topic_entity_id: number | null
  development_area_id: number | null
  name: string
  understanding_level_id: number
}

export interface Document {
  entity_id: number
  created_at: string
  updated_at: string
  version: number
  topic_id: number
  content: string
  completeness_level_id: number | null
  flashcard_status_id: number
  verification_status_id: number
  verification_level_id: number | null
  last_verified_at: string | null
}

export interface Flashcard {
  id: number
  document_id: number
  front: string
  back: string
  hint: string | null
  explanation: string | null
  state: number
  learning_step: number
  retention_levels_id: number
  stability: number | null
  difficulty: number | null
  last_reviewed_at: string | null
  due_at: string | null
  is_enabled: boolean
}

export interface CreateFlashcard {
  document_id: number
  front: string
  back: string
  hint: string | null
  explanation: string | null
  retention_levels_id: number
  is_enabled: boolean
}

export type UpdateFlashcard = Omit<CreateFlashcard, 'document_id'>

export interface CreateFlashcardResponse {
  id: number
}

export interface TopicGraphEntry {
  topic: Topic
  document: Document | null
  flashcards: Flashcard[]
}

export interface DevelopmentArea {
  id: number
  name: string
  background_color: string | null
  banner_image_path: string | null
  icon: string | null
  sort_order: number
}

export type CreateDevelopmentArea = Omit<DevelopmentArea, 'id'>
export type UpdateDevelopmentArea = CreateDevelopmentArea

export interface CreateDevelopmentAreaResponse {
  id: number
}

export interface TopicGraph {
  development_areas: DevelopmentArea[]
  topics: TopicGraphEntry[]
}
