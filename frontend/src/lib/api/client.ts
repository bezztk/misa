const API_BASE_URL = '/api';

export async function api<T>(endpoint: string, options: RequestInit = {}): Promise<T> {
    const response = await fetch(`${API_BASE_URL}${endpoint}`, {
        ...options, headers: {
            "Content-Type": "application/json",
            ...options.headers}
        });

    if (!response.ok) {
        throw new Error(response.statusText);
    }

    if (response.status === 204) {
        return undefined as T;
    }

    return await response.json() as Promise<T>;
}