import router from '@ohos.router';
import { common } from '@kit.AbilityKit';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { SurfaceManager, ISurfaceManagerListener, Surface } from '@agenui/agenui';
import { AGenUISurfaceCard } from '../components/AGenUISurfaceCard';

const TAG = 'WaterfallDemoPage';
const DOMAIN = 0x0000;
const API_VERSION = 'v0.9';
const CATALOG_URL = 'https://a2ui.org/specification/v0_9/standard_catalog.json';
const COLOR_PRIMARY = '#6200EE';
const COLOR_BG = '#F0F0F0';

interface MockCard {
  title: string;
  body: string;
  bgColor: string;
}

const MOCK_CARDS: MockCard[] = [
  {
    title: 'Weather Summary',
    body: 'Sunny, 24°C with low humidity. A perfect day for outdoor activities. UV index is moderate — sunscreen recommended.',
    bgColor: '#F0F8FF'
  },
  {
    title: 'Quick Stats',
    body: '12 active tasks. 3 due today. 2 overdue.',
    bgColor: '#FFFFFF'
  },
  {
    title: 'Daily Digest',
    body: 'Top stories this morning: AI regulation bill advances in the senate, local elections wrap up with record turnout, new transit line opens downtown, and global markets show mixed signals heading into the weekend. Subscribe for the evening edition.',
    bgColor: '#FFF8F0'
  },
  {
    title: 'Reminder',
    body: 'Team standup at 10:00 AM.',
    bgColor: '#FFFFFF'
  },
  {
    title: 'Activity Report',
    body: '8,432 steps today. 5.2 km walked. 420 calories burned. Active minutes: 47. Weekly goal: 80% complete. Great progress — keep it up!',
    bgColor: '#F0FFF4'
  },
  {
    title: 'Progress Update',
    body: 'Sprint 14 is 68% complete. 11 of 16 tasks done. 2 tasks blocked pending design review. Velocity is trending above average. Review meeting scheduled for Friday at 3 PM.',
    bgColor: '#FFFFFF'
  },
  {
    title: 'Note',
    body: 'Pick up dry cleaning before 6 PM.',
    bgColor: '#FFFEF0'
  },
  {
    title: 'Travel Guide: Kyoto',
    body: 'Spring in Kyoto is breathtaking. Cherry blossoms typically peak late March to early April. Must-see spots include Maruyama Park, Philosopher Path, Fushimi Inari, and the Arashiyama bamboo grove. Book accommodation 3+ months early — rooms fill up fast during hanami season.',
    bgColor: '#F0F8FF'
  },
  {
    title: 'Recipe: Avocado Toast',
    body: 'Toast sourdough until golden. Mash a ripe avocado with lemon juice, salt, and red pepper flakes. Spread generously on toast. Top with a soft-poached egg and a handful of microgreens. Drizzle with good olive oil. Season to taste. Serves 2.',
    bgColor: '#F0FFF4'
  },
  {
    title: 'Book Note',
    body: 'Atomic Habits by James Clear argues that 1% improvements compound dramatically over time. Focus on systems, not goals. Identity-based habits outlast outcome-based ones: become the person who exercises daily rather than trying to lose weight. Highly recommended.',
    bgColor: '#F8F0FF'
  },
  {
    title: 'Meeting Notes',
    body: 'Discussed Q3 roadmap. Payments feature deferred to Q4. New onboarding flow approved for A/B test starting next sprint. Design review set for Tuesday. Action: send mockups to stakeholders by EOD Thursday.',
    bgColor: '#FFF8F0'
  },
  {
    title: 'Inbox',
    body: '4 unread messages.',
    bgColor: '#FFFFFF'
  },
  {
    title: 'Workout Log',
    body: 'Morning run: 5.1 km in 28 min. Afternoon: 3 sets pull-ups (12, 10, 9), 3 sets dips (15, 14, 12), planks 3x60s. Evening stretch routine 15 min. Total active time: 74 minutes. Scheduled rest day tomorrow.',
    bgColor: '#F0FFF4'
  },
  {
    title: 'Quote of the Day',
    body: 'The best time to plant a tree was 20 years ago. The second best time is now.',
    bgColor: '#FFFEF0'
  },
  {
    title: 'Product Review',
    body: 'Sony WH-1000XM5 — exceptional noise cancellation, noticeably better than XM4. 30-hour battery holds up in real daily use. Comfortable for 4+ hour sessions. Call quality is much improved. The bass is slightly less punchy than previous gen but overall a clear upgrade. Highly recommended.',
    bgColor: '#F8F0FF'
  },
  {
    title: 'Status',
    body: 'All systems operational.',
    bgColor: '#FFFFFF'
  },
  {
    title: 'Learning Streak',
    body: 'Day 23 of your language learning streak! You have learned 480 new words this month. Your pronunciation score improved by 12% this week. Next lesson: the subjunctive mood in Spanish. Keep the momentum going!',
    bgColor: '#F0F8FF'
  },
  {
    title: 'Budget Summary',
    body: 'Monthly spend: $2,840 of $3,200 budget. Remaining: $360. Over budget: dining (+$45), entertainment (+$22). Under budget: transport (-$88), utilities (-$60). On track overall.',
    bgColor: '#FFF8F0'
  },
  {
    title: 'Tip',
    body: 'Stay hydrated. Aim for 8 glasses of water daily.',
    bgColor: '#F0FFF4'
  },
  {
    title: 'Top Stories',
    body: 'New battery tech promises 2x energy density at half the cost. Urban farming startup raises $40M Series B. Mars mission launch window confirmed for 2027. Open-source LLM surpasses proprietary benchmarks on reasoning tasks.',
    bgColor: '#F8F0FF'
  },
];

function buildCardJson(surfaceId: string, card: MockCard): string {
  const title = card.title.replace(/\\/g, '\\\\').replace(/"/g, '\\"');
  const body = card.body.replace(/\\/g, '\\\\').replace(/"/g, '\\"');
  return `{"version":"${API_VERSION}","updateComponents":{"surfaceId":"${surfaceId}","components":[` +
    `{"id":"root","component":"Column","children":["t","b"],"align":"stretch",` +
    `"styles":{"padding":"20px","gap":"12px","background-color":"${card.bgColor}"}},` +
    `{"id":"t","component":"Text","text":"${title}",` +
    `"styles":{"font-size":"30px","font-weight":"bold","line-height":"42px","color":"#1A1A1A","font-family":"system","line-clamp":0,"text-align":"left"}},` +
    `{"id":"b","component":"Text","text":"${body}",` +
    `"styles":{"font-size":"26px","line-height":"38px","color":"#444444","font-family":"system","line-clamp":0,"text-align":"left"}}` +
    `]}}`;
}

class WaterfallSurfaceListener implements ISurfaceManagerListener {
  private page: WaterfallDemoPage;

  constructor(page: WaterfallDemoPage) {
    this.page = page;
  }

  onCreateSurface(surface: Surface): void {
    hilog.info(DOMAIN, TAG, 'onCreateSurface: %{public}s', surface.surfaceId);
    this.page.onSurfaceReady(surface.surfaceId);
  }

  onDeleteSurface(surface: Surface): void {
    hilog.info(DOMAIN, TAG, 'onDeleteSurface: %{public}s', surface.surfaceId);
  }

  onError(surface: Surface | null, code: number, message: string): void {
    const sid = surface ? surface.surfaceId : '<none>';
    hilog.error(DOMAIN, TAG, 'surface error sid=%{public}s code=%{public}d: %{public}s', sid, code, message);
  }
}

@Entry
@Component
struct WaterfallDemoPage {
  @State surfaceIds: string[] = [];
  @State debugLabel: string = 'initializing...';
  private loadCount: number = 0;
  private pendingCount: number = 0;
  private isLoading: boolean = false;
  private surfaceManager: SurfaceManager | null = null;

  onSurfaceReady(surfaceId: string): void {
    this.surfaceIds = [...this.surfaceIds, surfaceId];
    this.pendingCount = this.pendingCount > 0 ? this.pendingCount - 1 : 0;
    if (this.pendingCount === 0) {
      this.isLoading = false;
    }
    this.debugLabel = `cards: ${this.surfaceIds.length}, pending: ${this.pendingCount}`;
    hilog.info(DOMAIN, TAG, 'onSurfaceReady total=%{public}d pending=%{public}d', this.surfaceIds.length, this.pendingCount);
  }

  aboutToAppear(): void {
    hilog.info(DOMAIN, TAG, 'aboutToAppear');
    const context = getContext(this) as common.UIAbilityContext;
    this.surfaceManager = new SurfaceManager(context);
    this.surfaceManager.addListener(new WaterfallSurfaceListener(this));
    hilog.info(DOMAIN, TAG, 'SurfaceManager ready, calling loadMore(12)');
    this.debugLabel = 'SurfaceManager created, loading...';
    this.loadMore(12);
  }

  aboutToDisappear(): void {
    hilog.info(DOMAIN, TAG, 'aboutToDisappear, destroying SurfaceManager');
    if (this.surfaceManager !== null) {
      this.surfaceManager.destroy();
      this.surfaceManager = null;
    }
  }

  private loadMore(n: number): void {
    if (this.isLoading) {
      hilog.warn(DOMAIN, TAG, 'loadMore(%{public}d) skipped: already loading', n);
      return;
    }
    if (this.surfaceManager === null) {
      hilog.error(DOMAIN, TAG, 'loadMore(%{public}d) skipped: surfaceManager is null', n);
      return;
    }
    this.isLoading = true;
    this.pendingCount += n;
    hilog.info(DOMAIN, TAG, 'loadMore n=%{public}d loadCount=%{public}d pending=%{public}d', n, this.loadCount, this.pendingCount);

    this.surfaceManager.beginTextStream();
    for (let i = 0; i < n; i++) {
      const idx = this.loadCount;
      const surfaceId = `wf_card_${idx}`;
      const card = MOCK_CARDS[idx % MOCK_CARDS.length];
      this.loadCount++;

      const createMsg =
        `{"version":"${API_VERSION}","createSurface":{"surfaceId":"${surfaceId}","catalogId":"${CATALOG_URL}"}}`;
      const updateMsg = buildCardJson(surfaceId, card);

      hilog.info(DOMAIN, TAG, 'sending surfaceId=%{public}s title=%{public}s', surfaceId, card.title);
      this.surfaceManager!.receiveTextChunk(createMsg);
      this.surfaceManager!.receiveTextChunk(updateMsg);
    }
    this.surfaceManager.endTextStream();
    hilog.info(DOMAIN, TAG, 'endTextStream done, waiting for onCreateSurface callbacks');
  }

  build() {
    Column() {
      Row() {
        Text('←')
          .fontSize(22)
          .fontColor(Color.White)
          .margin({ left: 16, right: 8 })
          .onClick(() => {
            router.back();
          })
        Text('Waterfall Demo')
          .fontSize(18)
          .fontWeight(FontWeight.Medium)
          .fontColor(Color.White)
          .layoutWeight(1)
        Text(this.debugLabel)
          .fontSize(11)
          .fontColor('#CCBBFF')
          .margin({ right: 12 })
      }
      .width('100%')
      .height(56)
      .backgroundColor(COLOR_PRIMARY)
      .alignItems(VerticalAlign.Center)

      WaterFlow() {
        ForEach(this.surfaceIds, (id: string) => {
          FlowItem() {
            AGenUISurfaceCard({ surfaceId: id })
          }
        }, (id: string) => id)
      }
      .columnsTemplate('1fr 1fr')
      .columnsGap(12)
      .rowsGap(12)
      .padding(12)
      .layoutWeight(1)
      .backgroundColor(COLOR_BG)
      .onReachEnd(() => {
        hilog.info(DOMAIN, TAG, 'onReachEnd, isLoading=%{public}s', String(this.isLoading));
        if (!this.isLoading) {
          this.loadMore(8);
        }
      })
    }
    .width('100%')
    .height('100%')
  }
}
