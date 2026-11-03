import { api } from "./client";

export interface Task {
    id: number;
    title: string;
}

export function getTasks(): Promise<Task[]> {
    return api<Task[]>("/tasks");
}