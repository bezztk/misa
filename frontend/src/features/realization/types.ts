export interface CreateTopic {
  parent_topic_entity_id: number | null
  name: string
  understanding_level_id: number
}

export interface CreateTopicResponse {
  entity_id: number
}
