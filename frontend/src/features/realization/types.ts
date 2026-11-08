export interface CreateTopic {
  parent_topic_entity_id: number | null
  name: string
}

export interface CreateTopicResponse {
  entity_id: number
}
