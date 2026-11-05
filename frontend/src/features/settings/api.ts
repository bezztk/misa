import { useMutation, useQueryClient } from '@tanstack/react-query'
import { apiClient } from '../../api/client'

function seedDevelopmentData() {
  return apiClient.post<void>('/dev/seed', {})
}

export function useSeedDevelopmentData() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: seedDevelopmentData,
    onSuccess: async () => {
      await Promise.all([
        queryClient.invalidateQueries({ queryKey: ['tasks'] }),
        queryClient.invalidateQueries({ queryKey: ['calendar'] }),
      ])
    },
  })
}
