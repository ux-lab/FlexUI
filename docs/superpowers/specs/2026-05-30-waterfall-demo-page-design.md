# Waterfall Demo Page — Design Spec

**Date:** 2026-05-30  
**Status:** Approved

---

## Overview

Add a `WaterfallDemoPage` to the HarmonyOS playground that demonstrates rendering multiple `AGenUIContainer` cards in a two-column waterfall layout. Cards load lazily (12 on enter, +8 per scroll-end). Each card is rendered by a new `AGenUISurfaceCard` component.

---

## Files

| File | Action |
|------|--------|
| `playground/harmony/entry/src/main/ets/pages/WaterfallDemoPage.ets` | New page |
| `playground/harmony/entry/src/main/ets/components/AGenUISurfaceCard.ets` | New component |
| `playground/harmony/entry/src/main/resources/base/profile/main_pages.json` | Add `pages/WaterfallDemoPage` |
| `playground/harmony/entry/src/main/ets/pages/AGenUIDemoPage.ets` | Add "Waterfall" nav button to toolbar |

---

## Component: `AGenUISurfaceCard`

A stateless presentation component. Takes one prop: `surfaceId: string`.

Renders `AGenUIContainer` inside a card shell:
- `border-radius`: 12 vp
- `clip`: true
- Background: white
- Shadow: `offsetY 4, blur 12, color rgba(0,0,0,0.08)`
- Width: `100%`; height auto (driven by the surface content)

No lifecycle logic. The parent page owns surface creation and destruction.

```
AGenUISurfaceCard
  └─ Column
       └─ AGenUIContainer({ surfaceId })
               .width('100%')
```

---

## Page: `WaterfallDemoPage`

### State

| Field | Type | Purpose |
|-------|------|---------|
| `surfaceIds` | `string[]` | Ordered list of active surface IDs rendered in the waterfall |
| `loadCount` | `number` | Monotonic counter; ensures unique surface IDs across loads |
| `isLoading` | `boolean` | Guard: prevents concurrent `onReachEnd` triggers |
| `surfaceManager` | `SurfaceManager \| null` | Single engine instance for all cards |
| `measurementManager` | `MeasurementManager \| null` | Shared measurement for custom components |

### Mock Data Pool

An inline array of card payload templates (minimum 20 entries). Templates vary in:

- **Title**: distinct short phrases (e.g. "Weather Summary", "Daily Digest", "Quick Stats", "Top Stories", "Activity Report", …)
- **Body length**: short (1–2 lines), medium (3–4 lines), long (6–8 lines) — natural height variation drives the waterfall stagger
- **Background tint**: white, light-blue (`#F0F8FF`), light-green (`#F0FFF0`), light-orange (`#FFF8F0`), light-purple (`#F8F0FF`)
- **Optional second section**: some cards include a sub-section (two-part layout) for extra height contrast

Each template is a complete `updateComponents` JSON payload string with a placeholder `surfaceId` that is filled in at load time. The `surfaceId` in the payload is set to the generated card surfaceId.

### Lifecycle

**`aboutToAppear`:**
1. Create `SurfaceManager(context)` and `MeasurementManager(engineId)`.
2. Register extensions: `ToastFunction`, `OpenUrlFunction`, `CustomMarkdownComponent`, `CustomChartComponent`, `CustomLottieComponent` (same set as `AGenUIDemoPage`).
3. Call `this.loadMore(12)`.

**`aboutToDisappear`:**
1. Destroy `surfaceManager` (tears down all surfaces and the C++ engine).
2. Unregister extensions.

**`loadMore(n: number)`:**
1. Guard: if `isLoading`, return immediately.
2. Set `isLoading = true`.
3. For each of `n` new cards:
   - Select template: `MOCK_TEMPLATES[(loadCount + offset) % MOCK_TEMPLATES.length]` with slight body-length variation to avoid identical adjacent cards.
   - Generate `surfaceId = "waterfall_card_${loadCount++}"`.
   - Inject `surfaceId` into the template JSON string.
   - Call `surfaceManager.beginTextStream()`.
   - Send `createSurface` message: `{"version":"v0.9","createSurface":{"surfaceId":"...","catalogId":"..."}}`.
   - Send `updateComponents` payload.
   - Call `surfaceManager.endTextStream()`.
4. Append all new surfaceIds to `surfaceIds` (single state update to batch re-render).
5. Set `isLoading = false`.

### Layout

```
Column
  └─ Row (toolbar)
  │    ├─ Back button  →  router.back()
  │    └─ Title: "Waterfall Demo"
  └─ WaterFlow
       .columnsTemplate("1fr 1fr")
       .columnsGap(12)        // 12 vp between the two columns — not doubled
       .rowsGap(12)           // 12 vp between consecutive rows in same column
       .padding({ left: 12, right: 12, top: 12, bottom: 12 })
       .layoutWeight(1)
       .onReachEnd(() => { if (!this.isLoading) this.loadMore(8) })
       └─ ForEach(this.surfaceIds, (id: string) =>
            FlowItem()
              └─ AGenUISurfaceCard({ surfaceId: id })
          , (id: string) => id)
```

**Gap collapse:** `columnsGap` and `rowsGap` on `WaterFlow` are single inter-item gaps — they do not double at shared boundaries. No per-item margins are needed.

---

## Navigation from AGenUIDemoPage

Add a "Waterfall" text button to the right side of the existing toolbar (between the dark-mode toggle and "Edit"):

```typescript
Text('Waterfall')
  .fontSize(16)
  .fontColor(Color.White)
  .margin({ right: 12 })
  .onClick(() => {
    router.pushUrl({ url: 'pages/WaterfallDemoPage' });
  })
```

Import `router` from `@kit.ArkUI`.

---

## main_pages.json Update

```json
{
  "src": [
    "pages/AGenUIDemoPage",
    "pages/WaterfallDemoPage"
  ]
}
```

---

## Out of Scope

- Dark mode support on `WaterfallDemoPage` (can be added later)
- Pull-to-refresh
- Card tap actions / detail navigation
- iOS and Android equivalents
