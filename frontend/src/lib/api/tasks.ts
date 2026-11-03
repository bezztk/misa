import { api } from "./client";
import type { Task, CreateTask } from "../models/task";


export function getTasks(): Promise<Task[]> {
    return api<Task[]>("/tasks");
}

export function createTask(task: CreateTask): Promise<Task> {
    return api<Task>("/tasks", {
        method: "POST",
        body: JSON.stringify(task),
    })
}