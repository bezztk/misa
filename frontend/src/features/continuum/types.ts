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
