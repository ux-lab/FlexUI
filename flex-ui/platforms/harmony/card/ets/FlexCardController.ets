import flexui from 'libflexui.so';

export interface FlexCardControllerOptions {
  bundleUri?: string;
  bundle?: string;
  bundleType: 'card-js' | 'a2ui-json';
  initialData?: object;
}

export class FlexCardController {
  private id: number;
  private destroyed: boolean = false;

  constructor(opts: FlexCardControllerOptions) {
    this.id = flexui.flexCardControllerCreate(JSON.stringify(opts));
  }

  load(): boolean {
    if (this.destroyed) return false;
    return flexui.flexCardControllerLoad(this.id);
  }

  setData(data: object): boolean {
    if (this.destroyed) return false;
    return flexui.flexCardControllerSetData(this.id, JSON.stringify(data));
  }

  destroy(): void {
    if (this.destroyed) return;
    flexui.flexCardControllerDestroy(this.id);
    this.destroyed = true;
  }

  _attach(nodeContent: NodeContent): void {
    flexui.flexCardControllerAttach(this.id, nodeContent);
  }

  _detach(): void {
    flexui.flexCardControllerDetach(this.id);
  }

  _id(): number { return this.id; }
}
