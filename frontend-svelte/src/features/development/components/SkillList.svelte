<script lang="ts">
    import {onMount} from "svelte";
    import {
        createSkill,
        deleteSkill,
        getSkillMetadata,
        getSkills,
        updateSkill,
    } from "../../../lib/api/skills";
    import type {Skill, SkillMetadata} from "../../../lib/models/skill";

    let {areaId}: {areaId: number} = $props();

    let skills: Skill[] = $state([]);
    let metadata: SkillMetadata | null = $state(null);
    let editingSkill: Skill | null = $state(null);
    let name = $state("");
    let description = $state("");
    let stateId: number | null = $state(null);
    let error = $state("");
    let formError = $state("");
    let pending = $state(false);
    let dialog: HTMLDialogElement;

    async function loadSkills() {
        try {
            error = "";
            metadata = await getSkillMetadata();
            skills = (await getSkills()).filter(skill => skill.area_id === areaId);
        } catch (err) {
            error = String(err);
        }
    }

    function openCreate() {
        error = "";
        formError = "";
        editingSkill = null;
        name = "";
        description = "";
        stateId = metadata?.states[0]?.id ?? null;
        dialog.showModal();
    }

    function openEdit(skill: Skill) {
        error = "";
        formError = "";
        editingSkill = skill;
        name = skill.name;
        description = skill.description ?? "";
        stateId = skill.state_id;
        dialog.showModal();
    }

    async function submit() {
        if (pending) return;

        try {
            formError = "";
            pending = true;

            if (editingSkill) {
                await updateSkill(editingSkill.item_id, {
                    name,
                    description,
                    state_id: stateId,
                });
            } else {
                await createSkill({
                    area_id: areaId,
                    name,
                    description,
                    state_id: stateId,
                });
            }

            dialog.close();
            await loadSkills();
        } catch (err) {
            formError = String(err);
        } finally {
            pending = false;
        }
    }

    async function remove(skill: Skill) {
        if (pending || !confirm(`Skill "${skill.name}" löschen?`)) return;

        try {
            error = "";
            pending = true;
            await deleteSkill(skill.item_id);
            await loadSkills();
        } catch (err) {
            error = String(err);
        } finally {
            pending = false;
        }
    }

    onMount(() => {
        void loadSkills();
    });
</script>

<section>
    <h3>Skills</h3>
    <button onclick={openCreate} disabled={pending}>Neuer Skill</button>

    {#if error}
        <p>{error}</p>
    {/if}

    {#each skills as skill (skill.item_id)}
        <article>
            <strong>{skill.name}</strong>
            <span>Status: {metadata?.states.find(state => state.id === skill.state_id)?.name}</span>
            <button onclick={() => openEdit(skill)} disabled={pending}>Bearbeiten</button>
            <button onclick={() => remove(skill)} disabled={pending}>Löschen</button>

            {#if skill.description}
                <p>{skill.description}</p>
            {/if}
        </article>
    {/each}
</section>

<dialog bind:this={dialog}>
    <form onsubmit={(event) => {
        event.preventDefault();
        void submit();
    }}>
        <h3>{editingSkill ? "Skill bearbeiten" : "Neuer Skill"}</h3>

        <label for="skill-name">Name</label>
        <input id="skill-name" bind:value={name} required />

        <label for="skill-state">Status</label>
        <select id="skill-state" bind:value={stateId} required>
            {#each metadata?.states ?? [] as state}
                <option value={state.id}>{state.name}</option>
            {/each}
        </select>

        <label for="skill-description">Beschreibung</label>
        <textarea id="skill-description" bind:value={description}></textarea>

        {#if formError}
            <p>{formError}</p>
        {/if}

        <button type="submit" disabled={pending}>
            {pending ? "Wird gespeichert..." : "Speichern"}
        </button>
        <button type="button" onclick={() => dialog.close()}>Abbrechen</button>
    </form>
</dialog>

<style>
    article {
        padding: 1rem 0;
        border-bottom: 1px solid #ddd;
    }

    article span,
    article button {
        margin-left: 0.5rem;
    }

    form {
        display: flex;
        flex-direction: column;
        gap: 0.5rem;
    }
</style>
