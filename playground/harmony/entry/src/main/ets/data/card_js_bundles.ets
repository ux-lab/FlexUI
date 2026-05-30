export interface CardBundle {
  type: 'card-js' | 'a2ui-json';
  source: string;
  initialData: object;
  label: string;
}

const titles = ['Weather Summary', 'Daily Digest', 'Quick Stats',
                'Top Stories', 'Activity Report', 'Order Update'];
const bodies = [
  'Sunny with a chance of meatballs.',
  'Three minutes to read. Five sections.',
  'Up 17%, down 3, holding steady.',
  'Latest events from your network.',
  'Today: 8.2km, 11k steps, 540 kcal.',
  'Order #12345 shipped 14:32.',
];

export const CARD_JS_BUNDLES: CardBundle[] = titles.map((title, i) => ({
  type: 'card-js',
  label: `card-js #${i + 1}: ${title}`,
  initialData: { title, body: bodies[i % bodies.length] },
  source: `
    function render(data) {
      return createElement('View',
        { padding: 12, backgroundColor: '#FFFFFF', borderRadius: 12 },
        [
          createElement('Text',
            { text: data.title, fontSize: 18, fontWeight: 'bold' }),
          createElement('Text',
            { text: data.body, fontSize: 14, color: '#666666', maxLines: 6 }),
          createElement('Button',
            { title: 'Tap', onClick: 'tap', backgroundColor: '#007AFF' })
        ]);
    }
    globalThis.__flexui_exports = {
      render,
      handlers: {
        tap: function() { console.log('card-js tap on ' + globalThis.__data?.title); }
      }
    };
  `,
}));
