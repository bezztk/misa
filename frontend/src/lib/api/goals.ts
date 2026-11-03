import {api} from "./client";
import type {CreateGoal, Goal, UpdateGoal} from "../models/goal";

export function getGoals(): Promise<Goal[]> {
    return api<Goal[]>("/goals");
}

export function getGoal(id: number): Promise<Goal> {
    return api<Goal>(`/goals/${id}`);
}

export function createGoal(goal: CreateGoal): Promise<{ id: number }> {
    return api<{ id: number }>("/goals", {
        method: "POST",
        body: JSON.stringify(goal),
    });
}

export function updateGoal(id: number, goal: UpdateGoal): Promise<void> {
    return api<void>(`/goals/${id}`, {
        method: "PATCH",
        body: JSON.stringify(goal),
    });
}

export function deleteGoal(id: number): Promise<void> {
    return api<void>(`/goals/${id}`, {
        method: "DELETE",
    });
}
