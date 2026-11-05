const API_BASE_URL = (import.meta.env.VITE_API_BASE_URL ?? '/api').replace(
  /\/$/,
  '',
)

type HttpMethod = 'GET' | 'POST' | 'PUT' | 'DELETE'

async function request<T>(path: string, method: HttpMethod, body?: unknown) {
  const response = await fetch(`${API_BASE_URL}${path}`, {
    method,
    headers: {
      Accept: 'application/json',
      ...(body === undefined ? {} : { 'Content-Type': 'application/json' }),
    },
    body: body === undefined ? undefined : JSON.stringify(body),
  })

  if (!response.ok) {
    throw new Error(`API request failed with status ${response.status}`)
  }

  if (response.status === 204) {
    return undefined as T
  }

  return response.json() as Promise<T>
}

export const apiClient = {
  get<T>(path: string) {
    return request<T>(path, 'GET')
  },
  post<T>(path: string, body: unknown) {
    return request<T>(path, 'POST', body)
  },
  put<T>(path: string, body: unknown) {
    return request<T>(path, 'PUT', body)
  },
  delete<T = void>(path: string) {
    return request<T>(path, 'DELETE')
  },
}
