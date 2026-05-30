import { AGenUIContainer } from '@agenui/agenui';

@Component
export struct AGenUISurfaceCard {
  surfaceId: string = '';

  build() {
    Column() {
      AGenUIContainer({ surfaceId: this.surfaceId })
        .width('100%')
    }
    .width('100%')
    .borderRadius(12)
    .clip(true)
    .backgroundColor(Color.White)
    .shadow({ radius: 12, color: '#14000000', offsetX: 0, offsetY: 4 })
  }
}
