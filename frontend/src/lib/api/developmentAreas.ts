import {api} from "./client";
import type {
    CreateDevelopmentArea,
    DevelopmentArea,
    UpdateDevelopmentArea,
} from "../models/developmentArea";

export function getDevelopmentAreas(): Promise<DevelopmentArea[]> {
    return api<DevelopmentArea[]>("/development-areas");
}

export function createDevelopmentArea(area: CreateDevelopmentArea): Promise<{ id: number }> {
    return api<{ id: number }>("/development-areas", {
        method: "POST",
        body: JSON.stringify(area),
    });
}

export function updateDevelopmentArea(id: number, area: UpdateDevelopmentArea): Promise<void> {
    return api<void>(`/development-areas/${id}`, {
        method: "PATCH",
        body: JSON.stringify(area),
    });
}

export function deleteDevelopmentArea(id: number): Promise<void> {
    return api<void>(`/development-areas/${id}`, {
        method: "DELETE",
    });
}
