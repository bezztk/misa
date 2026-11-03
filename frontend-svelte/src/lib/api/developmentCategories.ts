import {api} from "./client";
import type {
    CreateDevelopmentCategory,
    DevelopmentCategory,
    UpdateDevelopmentCategory,
} from "../models/developmentCategory";

export function getDevelopmentCategories(): Promise<DevelopmentCategory[]> {
    return api<DevelopmentCategory[]>("/development-categories");
}

export function getDevelopmentCategory(id: number): Promise<DevelopmentCategory> {
    return api<DevelopmentCategory>(`/development-categories/${id}`);
}

export function createDevelopmentCategory(
    category: CreateDevelopmentCategory,
): Promise<{ id: number }> {
    return api<{ id: number }>("/development-categories", {
        method: "POST",
        body: JSON.stringify(category),
    });
}

export function updateDevelopmentCategory(
    id: number,
    category: UpdateDevelopmentCategory,
): Promise<void> {
    return api<void>(`/development-categories/${id}`, {
        method: "PATCH",
        body: JSON.stringify(category),
    });
}

export function deleteDevelopmentCategory(id: number): Promise<void> {
    return api<void>(`/development-categories/${id}`, {
        method: "DELETE",
    });
}
