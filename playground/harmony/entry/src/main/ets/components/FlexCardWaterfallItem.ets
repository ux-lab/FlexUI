import { FlexCard, FlexCardController } from '@flexui/host-card';

@Component
export struct FlexCardWaterfallItem {
  @ObjectLink controller: FlexCardController;
  @Prop label: string = '';

  build() {
    Column() {
      FlexCard({ controller: this.controller })
    }
    .width('100%')
    .borderRadius(12)
    .clip(true)
    .backgroundColor(Color.White)
    .shadow({ radius: 12, color: '#0F000000', offsetY: 4 })
    .margin({ bottom: 12 })
  }
}
