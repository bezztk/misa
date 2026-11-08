export interface CreateTopic {
  parent_topic_entity_id: number | null
  name: string
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

export interface TopicGraphEntry {
  topic: Topic
  document: Document | null
}
