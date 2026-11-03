import {api} from "./client";
import type {ItemHierarchy} from "../models/item";

export function getItemHierarchy(): Promise<ItemHierarchy[]> {
    return api<ItemHierarchy[]>("/items/hierarchy");
}
