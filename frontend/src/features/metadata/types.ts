export interface MetadataItem {
  id: number
  name: string
  display_name: string | null
}

export interface FlashcardRetentionLevel extends MetadataItem {
  desired_retention: number
}

export interface TaskGroup {
  id: number
  name: string
}

export interface Metadata {
  task_groups: TaskGroup[]
  task_states: MetadataItem[]
  task_priorities: MetadataItem[]
  topic_understanding_levels: MetadataItem[]
  document_completeness_levels: MetadataItem[]
  document_flashcard_statuses: MetadataItem[]
  document_verification_statuses: MetadataItem[]
  document_verification_levels: MetadataItem[]
  flashcard_retention_levels: FlashcardRetentionLevel[]
}
