import { CardBundle } from './card_js_bundles';

const titles = ['Trending', 'Recommended', 'Recent Activity',
                'New Releases', 'For You', 'Saved'];
const bodies = [
  'Sunny with a chance of meatballs.',
  'Three minutes to read. Five sections.',
  'Up 17%, down 3, holding steady.',
  'Latest events from your network.',
  'Today: 8.2km, 11k steps, 540 kcal.',
  'Order #12345 shipped 14:32.',
];

export const A2UI_JSON_BUNDLES: CardBundle[] = titles.map((title, i) => ({
  type: 'a2ui-json',
  label: `a2ui-json #${i + 1}: ${title}`,
  initialData: { title, body: bodies[i % bodies.length] },
  source: JSON.stringify({
    type: 'View',
    props: { padding: 12, backgroundColor: '#FFFFFF', borderRadius: 12 },
    children: [
      { type: 'Text', props: { text: '{{title}}', fontSize: 18, fontWeight: 'bold' } },
      { type: 'Text', props: { text: '{{body}}', fontSize: 14, color: '#666666', maxLines: 6 } },
      { type: 'Button', props: { title: 'Tap', onClick: 'tap', backgroundColor: '#34C759' } },
    ],
    handlersJs: "globalThis.__a2ui_handlers = { tap: function() { console.log('a2ui tap'); } };",
  }),
}));
