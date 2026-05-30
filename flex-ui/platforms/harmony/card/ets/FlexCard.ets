import { FlexCardController } from './FlexCardController';

@Component
export struct FlexCard {
  @ObjectLink controller: FlexCardController;
  private nodeContent: NodeContent = new NodeContent();

  aboutToAppear() {
    this.controller._attach(this.nodeContent);
  }

  aboutToDisappear() {
    this.controller._detach();
  }

  build() {
    ContentSlot(this.nodeContent)
      .width('100%')
      .height('auto');
  }
}
