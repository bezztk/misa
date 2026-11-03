<script lang="ts">
    import {onMount} from "svelte";
    import {ScheduleXCalendar} from "@schedule-x/svelte";
    import {
        createCalendar,
        createViewDay,
        createViewMonthGrid,
        createViewWeek,
    } from "@schedule-x/calendar";
    import "@schedule-x/theme-default/dist/index.css";
    import "temporal-polyfill/global";

    let calendarApp = $state<ReturnType<typeof createCalendar>>();

    onMount(() => {
        calendarApp = createCalendar({
            locale: "de-DE",
            timezone: "Europe/Berlin",
            views: [
                createViewDay(),
                createViewWeek(),
                createViewMonthGrid(),
            ],
            events: [],
        });
    });
</script>

<div class="calendar">
    {#if calendarApp}
        <ScheduleXCalendar {calendarApp} />
    {/if}
</div>

<style>
    .calendar {
        width: 100%;
        height: 100vh;
    }

    .calendar :global(.sx__calendar) {
        border: 0;
    }
</style>
