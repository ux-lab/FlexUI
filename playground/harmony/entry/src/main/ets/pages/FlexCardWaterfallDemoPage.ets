import router from '@ohos.router';
import { FlexUIEngine, FlexCardController, JsEngineBackend } from '@flexui/host-card';
import { FlexCardWaterfallItem } from '../components/FlexCardWaterfallItem';
import { CARD_JS_BUNDLES, CardBundle } from '../data/card_js_bundles';
import { A2UI_JSON_BUNDLES } from '../data/a2ui_json_bundles';

@Entry
@Component
struct FlexCardWaterfallDemoPage {
  @State activeControllers: FlexCardController[] = [];
  @State activeLabels: string[] = [];
  @State loadingMore: boolean = false;
  private nextBundleIdx: number = 0;
  private pool: CardBundle[] = [...CARD_JS_BUNDLES, ...A2UI_JSON_BUNDLES];

  aboutToAppear() {
    if (!FlexUIEngine.isInitialized()) {
      FlexUIEngine.init({ backend: JsEngineBackend.Jsvm });
    }
    this.loadMore(12);
  }

  aboutToDisappear() {
    for (const c of this.activeControllers) c.destroy();
    this.activeControllers = [];
    FlexUIEngine.shutdown();
  }

  private loadMore(n: number): void {
    this.loadingMore = true;
    const newControllers = [...this.activeControllers];
    const newLabels = [...this.activeLabels];
    for (let i = 0; i < n; ++i) {
      const b = this.pool[this.nextBundleIdx % this.pool.length];
      this.nextBundleIdx++;
      const c = new FlexCardController({
        bundle: b.source,
        bundleType: b.type,
        initialData: b.initialData,
      });
      c.load();
      newControllers.push(c);
      newLabels.push(b.label);
    }
    this.activeControllers = newControllers;
    this.activeLabels = newLabels;
    this.loadingMore = false;
  }

  build() {
    Column() {
      Row() {
        Button('← Back')
          .onClick(() => router.back())
        Text('FlexCard Waterfall Demo')
          .fontSize(18)
          .margin({ left: 8 })
        Blank()
        Button('Dump')
          .fontSize(14)
          .onClick(() => FlexUIEngine.dumpDiagnostics())
      }
      .width('100%')
      .padding(12)

      Grid() {
        ForEach(this.activeControllers, (controller: FlexCardController, idx: number) => {
          GridItem() {
            FlexCardWaterfallItem({
              controller: controller,
              label: this.activeLabels[idx],
            })
          }
        }, (controller: FlexCardController) => `card-${controller._id()}`)
      }
      .columnsTemplate('1fr 1fr')
      .columnsGap(12)
      .rowsGap(12)
      .padding(12)
      .layoutWeight(1)
      .onReachEnd(() => {
        if (this.loadingMore) return;
        this.loadMore(8);
      })
    }
    .width('100%')
    .height('100%')
    .backgroundColor('#F2F2F7')
  }
}
