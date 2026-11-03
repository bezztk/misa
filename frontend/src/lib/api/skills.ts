import {api} from "./client";
import type {CreateSkill, Skill, SkillMetadata, UpdateSkill} from "../models/skill";

export function getSkills(): Promise<Skill[]> {
    return api<Skill[]>("/skills");
}

export function getSkill(id: number): Promise<Skill> {
    return api<Skill>(`/skills/${id}`);
}

export function createSkill(skill: CreateSkill): Promise<{ id: number }> {
    return api<{ id: number }>("/skills", {
        method: "POST",
        body: JSON.stringify(skill),
    });
}

export function updateSkill(id: number, skill: UpdateSkill): Promise<void> {
    return api<void>(`/skills/${id}`, {
        method: "PATCH",
        body: JSON.stringify(skill),
    });
}

export function deleteSkill(id: number): Promise<void> {
    return api<void>(`/skills/${id}`, {
        method: "DELETE",
    });
}

export function getSkillMetadata(): Promise<SkillMetadata> {
    return api<SkillMetadata>("/skills/metadata");
}
