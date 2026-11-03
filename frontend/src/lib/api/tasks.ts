import { api } from "./client";
import type { Task, CreateTask } from "../models/task";
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

export function getTaskMetadata(): Promise<TaskMetadata> {
    return api<TaskMetadata>("/tasks/metadata");
}