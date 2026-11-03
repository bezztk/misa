import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query'
import { apiClient } from '../../api/client'
import type {
  AllowedTypeRelation,
  CreateDevelopmentArea,
  CreateDevelopmentAreaResponse,
  CreateDevelopmentCategory,
  CreateDevelopmentCategoryResponse,
  CreateGoal,
  CreateGoalResponse,
  CreateSkill,
  CreateSkillResponse,
  DevelopmentArea,
  DevelopmentCategory,
  Goal,
  ItemHierarchy,
  Skill,
  SkillMetadata,
  UpdateDevelopmentArea,
  UpdateDevelopmentCategory,
  UpdateGoal,
  UpdateItemArchive,
  UpdateItemParent,
  UpdateSkill,
} from './types'

const keys = {
  developmentAreas: ['continuum', 'development-areas', 'list'] as const,
  developmentArea: (id: number) =>
    ['continuum', 'development-areas', 'detail', id] as const,
  developmentCategories: [
    'continuum',
    'development-categories',
    'list',
  ] as const,
  developmentCategory: (id: number) =>
    ['continuum', 'development-categories', 'detail', id] as const,
  skills: ['continuum', 'skills', 'list'] as const,
  skill: (id: number) => ['continuum', 'skills', 'detail', id] as const,
  skillMetadata: ['continuum', 'skills', 'metadata'] as const,
  goals: ['continuum', 'goals', 'list'] as const,
  goal: (id: number) => ['continuum', 'goals', 'detail', id] as const,
  itemHierarchy: ['continuum', 'items', 'hierarchy'] as const,
  allowedTypeRelations: ['continuum', 'items', 'allowed-relations'] as const,
}

export function getDevelopmentAreas() {
  return apiClient.get<DevelopmentArea[]>('/development-areas')
}

export function getDevelopmentArea(id: number) {
  return apiClient.get<DevelopmentArea>(`/development-areas/${id}`)
}

export function createDevelopmentArea(area: CreateDevelopmentArea) {
  return apiClient.post<CreateDevelopmentAreaResponse>(
    '/development-areas',
    area,
  )
}

export function updateDevelopmentArea(
  id: number,
  area: UpdateDevelopmentArea,
) {
  return apiClient.patch<void>(`/development-areas/${id}`, area)
}

export function deleteDevelopmentArea(id: number) {
  return apiClient.delete(`/development-areas/${id}`)
}

export function useDevelopmentAreas(enabled = true) {
  return useQuery({
    queryKey: keys.developmentAreas,
    queryFn: getDevelopmentAreas,
    enabled,
  })
}

export function useDevelopmentArea(id: number) {
  return useQuery({
    queryKey: keys.developmentArea(id),
    queryFn: () => getDevelopmentArea(id),
  })
}

export function useCreateDevelopmentArea() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: createDevelopmentArea,
    onSuccess: () =>
      queryClient.invalidateQueries({ queryKey: keys.developmentAreas }),
  })
}

export function useUpdateDevelopmentArea() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: ({
      id,
      area,
    }: {
      id: number
      area: UpdateDevelopmentArea
    }) => updateDevelopmentArea(id, area),
    onSuccess: async (_, { id }) => {
      await Promise.all([
        queryClient.invalidateQueries({ queryKey: keys.developmentAreas }),
        queryClient.invalidateQueries({
          queryKey: keys.developmentArea(id),
        }),
      ])
    },
  })
}

export function useDeleteDevelopmentArea() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: deleteDevelopmentArea,
    onSuccess: async (_, id) => {
      queryClient.removeQueries({ queryKey: keys.developmentArea(id) })
      await queryClient.invalidateQueries({ queryKey: keys.developmentAreas })
    },
  })
}

export function getDevelopmentCategories() {
  return apiClient.get<DevelopmentCategory[]>('/development-categories')
}

export function getDevelopmentCategory(id: number) {
  return apiClient.get<DevelopmentCategory>(`/development-categories/${id}`)
}

export function createDevelopmentCategory(category: CreateDevelopmentCategory) {
  return apiClient.post<CreateDevelopmentCategoryResponse>(
    '/development-categories',
    category,
  )
}

export function updateDevelopmentCategory(
  id: number,
  category: UpdateDevelopmentCategory,
) {
  return apiClient.patch<void>(`/development-categories/${id}`, category)
}

export function deleteDevelopmentCategory(id: number) {
  return apiClient.delete(`/development-categories/${id}`)
}

export function useDevelopmentCategories() {
  return useQuery({
    queryKey: keys.developmentCategories,
    queryFn: getDevelopmentCategories,
  })
}

export function useDevelopmentCategory(id: number) {
  return useQuery({
    queryKey: keys.developmentCategory(id),
    queryFn: () => getDevelopmentCategory(id),
  })
}

export function useCreateDevelopmentCategory() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: createDevelopmentCategory,
    onSuccess: () =>
      queryClient.invalidateQueries({
        queryKey: keys.developmentCategories,
      }),
  })
}

export function useUpdateDevelopmentCategory() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: ({
      id,
      category,
    }: {
      id: number
      category: UpdateDevelopmentCategory
    }) => updateDevelopmentCategory(id, category),
    onSuccess: async (_, { id }) => {
      await Promise.all([
        queryClient.invalidateQueries({
          queryKey: keys.developmentCategories,
        }),
        queryClient.invalidateQueries({
          queryKey: keys.developmentCategory(id),
        }),
      ])
    },
  })
}

export function useDeleteDevelopmentCategory() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: deleteDevelopmentCategory,
    onSuccess: async (_, id) => {
      queryClient.removeQueries({ queryKey: keys.developmentCategory(id) })
      await Promise.all([
        queryClient.invalidateQueries({
          queryKey: keys.developmentCategories,
        }),
        queryClient.invalidateQueries({ queryKey: keys.itemHierarchy }),
      ])
    },
  })
}

export function getSkills() {
  return apiClient.get<Skill[]>('/skills')
}

export function getSkill(id: number) {
  return apiClient.get<Skill>(`/skills/${id}`)
}

export function getSkillMetadata() {
  return apiClient.get<SkillMetadata>('/skills/metadata')
}

export function createSkill(skill: CreateSkill) {
  return apiClient.post<CreateSkillResponse>('/skills', skill)
}

export function updateSkill(id: number, skill: UpdateSkill) {
  return apiClient.patch<void>(`/skills/${id}`, skill)
}

export function deleteSkill(id: number) {
  return apiClient.delete(`/skills/${id}`)
}

export function useSkills() {
  return useQuery({ queryKey: keys.skills, queryFn: getSkills })
}

export function useSkill(id: number) {
  return useQuery({
    queryKey: keys.skill(id),
    queryFn: () => getSkill(id),
  })
}

export function useSkillMetadata() {
  return useQuery({
    queryKey: keys.skillMetadata,
    queryFn: getSkillMetadata,
  })
}

export function useCreateSkill() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: createSkill,
    onSuccess: () => queryClient.invalidateQueries({ queryKey: keys.skills }),
  })
}

export function useUpdateSkill() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: ({ id, skill }: { id: number; skill: UpdateSkill }) =>
      updateSkill(id, skill),
    onSuccess: async (_, { id }) => {
      await Promise.all([
        queryClient.invalidateQueries({ queryKey: keys.skills }),
        queryClient.invalidateQueries({ queryKey: keys.skill(id) }),
      ])
    },
  })
}

export function useDeleteSkill() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: deleteSkill,
    onSuccess: async (_, id) => {
      queryClient.removeQueries({ queryKey: keys.skill(id) })
      await Promise.all([
        queryClient.invalidateQueries({ queryKey: keys.skills }),
        queryClient.invalidateQueries({ queryKey: keys.itemHierarchy }),
      ])
    },
  })
}

export function getGoals() {
  return apiClient.get<Goal[]>('/goals')
}

export function getGoal(id: number) {
  return apiClient.get<Goal>(`/goals/${id}`)
}

export function createGoal(goal: CreateGoal) {
  return apiClient.post<CreateGoalResponse>('/goals', goal)
}

export function updateGoal(id: number, goal: UpdateGoal) {
  return apiClient.patch<void>(`/goals/${id}`, goal)
}

export function deleteGoal(id: number) {
  return apiClient.delete(`/goals/${id}`)
}

export function useGoals() {
  return useQuery({ queryKey: keys.goals, queryFn: getGoals })
}

export function useGoal(id: number) {
  return useQuery({
    queryKey: keys.goal(id),
    queryFn: () => getGoal(id),
  })
}

export function useCreateGoal() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: createGoal,
    onSuccess: () => queryClient.invalidateQueries({ queryKey: keys.goals }),
  })
}

export function useUpdateGoal() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: ({ id, goal }: { id: number; goal: UpdateGoal }) =>
      updateGoal(id, goal),
    onSuccess: async (_, { id }) => {
      await Promise.all([
        queryClient.invalidateQueries({ queryKey: keys.goals }),
        queryClient.invalidateQueries({ queryKey: keys.goal(id) }),
      ])
    },
  })
}

export function useDeleteGoal() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: deleteGoal,
    onSuccess: async (_, id) => {
      queryClient.removeQueries({ queryKey: keys.goal(id) })
      await Promise.all([
        queryClient.invalidateQueries({ queryKey: keys.goals }),
        queryClient.invalidateQueries({ queryKey: keys.itemHierarchy }),
      ])
    },
  })
}

export function getItemHierarchy() {
  return apiClient.get<ItemHierarchy[]>('/items/hierarchy')
}

export function getAllowedTypeRelations() {
  return apiClient.get<AllowedTypeRelation[]>('/items/allowed-type-relations')
}

export function updateItemParent(id: number, item: UpdateItemParent) {
  return apiClient.patch<void>(`/items/${id}/parent`, item)
}

export function updateItemArchive(id: number, item: UpdateItemArchive) {
  return apiClient.patch<void>(`/items/${id}/archive`, item)
}

export function useItemHierarchy() {
  return useQuery({
    queryKey: keys.itemHierarchy,
    queryFn: getItemHierarchy,
  })
}

export function useAllowedTypeRelations() {
  return useQuery({
    queryKey: keys.allowedTypeRelations,
    queryFn: getAllowedTypeRelations,
  })
}

export function useUpdateItemParent() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: ({ id, item }: { id: number; item: UpdateItemParent }) =>
      updateItemParent(id, item),
    onSuccess: async () => {
      await Promise.all([
        queryClient.invalidateQueries({ queryKey: keys.itemHierarchy }),
        queryClient.invalidateQueries({ queryKey: ['tasks'] }),
      ])
    },
  })
}

export function useUpdateItemArchive() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: ({ id, item }: { id: number; item: UpdateItemArchive }) =>
      updateItemArchive(id, item),
    onSuccess: () => queryClient.invalidateQueries({ queryKey: ['tasks'] }),
  })
}
