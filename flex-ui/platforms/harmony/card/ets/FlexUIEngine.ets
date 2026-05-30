import flexui from 'libflexui.so';

export enum JsEngineBackend {
  QuickJS = 0,
  Jsvm = 1,
}

export interface FlexUIEngineConfig {
  backend?: JsEngineBackend;
  forceQuickJsForDebug?: boolean;
}

export class FlexUIEngine {
  private static initialized: boolean = false;

  static init(cfg: FlexUIEngineConfig = {}): boolean {
    const backend = cfg.backend ?? JsEngineBackend.Jsvm;
    const ok = flexui.flexUiEngineInit(backend, !!cfg.forceQuickJsForDebug);
    FlexUIEngine.initialized = ok;
    return ok;
  }

  static install(plugin: object): boolean {
    // Real plugin install lands in W7-W8 when components/api/loader/frontend
    // bindings stabilize. For W5-W6, accept any plugin object and return true.
    return true;
  }

  static shutdown(): void {
    flexui.flexUiEngineShutdown();
    FlexUIEngine.initialized = false;
  }

  static isInitialized(): boolean {
    return FlexUIEngine.initialized;
  }
}
