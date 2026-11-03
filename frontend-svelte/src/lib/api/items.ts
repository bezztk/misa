import {api} from "./client";
import type {AllowedTypeRelation, ItemHierarchy} from "../models/item";

export function getItemHierarchy(): Promise<ItemHierarchy[]> {
    return api<ItemHierarchy[]>("/items/hierarchy");
}

export function getAllowedTypeRelations(): Promise<AllowedTypeRelation[]> {
    return api<AllowedTypeRelation[]>("/items/allowed-type-relations");
}

export function updateItemParent(id: number, parentId: number | null): Promise<void> {
    return api<void>(`/items/${id}/parent`, {
        method: "PATCH",
        body: JSON.stringify({parent_id: parentId}),
    });
}
