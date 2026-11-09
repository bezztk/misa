const API_BASE_URL = (import.meta.env.VITE_API_BASE_URL ?? '/api').replace(
  /\/$/,
  '',
)

type HttpMethod = 'GET' | 'POST' | 'PUT' | 'PATCH' | 'DELETE'

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

  const responseText = await response.text()
  return responseText === ''
    ? undefined as T
    : JSON.parse(responseText) as T
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
  patch<T>(path: string, body: unknown) {
    return request<T>(path, 'PATCH', body)
  },
  delete<T = void>(path: string) {
    return request<T>(path, 'DELETE')
  },
}
