import { api } from "./client";
import type { Task, CreateTask, UpdateTask } from "../models/task";
import type {TaskMetadata} from "../models/taskMetadata";


export function getTasks(): Promise<Task[]> {
    return api<Task[]>("/tasks");
}

export function createTask(task: CreateTask): Promise<Task> {
    return api<Task>("/tasks", {
        method: "POST",
        body: JSON.stringify(task),
    })
}

export function updateTask(id: number, task: UpdateTask): Promise<void> {
    return api<void>(`/tasks/${id}`, {
        method: "PATCH",
        body: JSON.stringify(task),
    })
}

export function getTaskMetadata(): Promise<TaskMetadata> {
    return api<TaskMetadata>("/tasks/metadata");
}
