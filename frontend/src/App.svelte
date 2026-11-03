<script lang="ts">
    import TaskList from "./features/tasks/components/TaskList.svelte";
    import DevelopmentView from "./features/development/components/DevelopmentView.svelte";
    import Calendar from "./features/calendar/components/Calendar.svelte";

    let view = $state<"tasks" | "development" | "calendar">("tasks");
</script>

<aside>
    <h1>Misa</h1>
    <nav>
        <button
                class:active={view === "tasks"}
                aria-current={view === "tasks" ? "page" : undefined}
                onclick={() => view = "tasks"}
        >
            <span aria-hidden="true">☑</span>
            Aufgaben
        </button>
        <button
                class:active={view === "development"}
                aria-current={view === "development" ? "page" : undefined}
                onclick={() => view = "development"}
        >
            <span aria-hidden="true">◆</span>
            Entwicklung
        </button>
        <button
                class:active={view === "calendar"}
                aria-current={view === "calendar" ? "page" : undefined}
                onclick={() => view = "calendar"}
        >
            <span aria-hidden="true">▦</span>
            Kalender
        </button>
    </nav>
</aside>

<main class:calendar-view={view === "calendar"}>
    {#if view === "tasks"}
        <TaskList />
    {:else if view === "development"}
        <DevelopmentView />
    {:else}
        <Calendar />
    {/if}
</main>

<style>
    aside {
        position: fixed;
        inset: 0 auto 0 0;
        width: 14rem;
        padding: 1.5rem 1rem;
        background: #f5f5f5;
        border-right: 1px solid #ddd;
    }

    aside h1 {
        margin: 0 0 2rem;
    }

    nav {
        display: flex;
        flex-direction: column;
        gap: 0.25rem;
    }

    nav button {
        display: flex;
        align-items: center;
        gap: 0.75rem;
        width: 100%;
        padding: 0.75rem;
        border: 0;
        border-radius: 0.25rem;
        background: transparent;
        text-align: left;
        cursor: pointer;
    }

    nav button:hover,
    nav button.active {
        background: #e3e3e3;
    }

    nav span {
        width: 1rem;
        text-align: center;
    }

    main {
        min-height: 100vh;
        margin-left: 14rem;
        padding: 2rem;
    }

    main.calendar-view {
        padding: 0;
    }
</style>
