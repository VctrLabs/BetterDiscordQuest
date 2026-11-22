# BetterDiscordQuest
# Discord Quest Automator

A refined, low-profile automation script for completing accepted Discord Quests. Unlike standard public snippets that rely on rigid timers and abrupt state changes, this version is built around natural pacing and realistic session behavior.

## Why Use This Over Standard Scripts?

Most publicly available quest scripts follow predictable, automated patterns that are easy to flag through basic telemetry checks. This version addresses those flaws at a behavioral level:

* **Natural Pacing & Long Cooldowns:** Standard scripts run through every accepted quest back-to-back without stopping. This script shuffles your quest queue and introduces randomized breaks (ranging from 10 minutes to 2 hours) between tasks so activity looks spread out over a normal day.
* **Realistic Timing & Jitter:** Instead of sending progress updates on exact, metronomic intervals (e.g., every `7.0s` or `20.0s`), all heartbeats and checks use randomized intervals.
* **True 1x Playback & Post-Completion Linger:** Many older snippets advance video progress faster than real-time (`>1.0x` speed) and kill simulated games the exact millisecond a quest hits 100%. This version tracks real elapsed time and keeps the simulated game or stream active for a brief, randomized cooldown period after completion before closing.
* **Clean Client State:** Existing running games are preserved rather than overwritten, process metadata follows realistic formatting, and execution is isolated so nothing is left exposed in the global window scope.
* **Resilient Execution:** Automatically re-checks quest availability and expiration after long breaks and recovers cleanly from temporary network hiccups.

## Supported Quest Types

* `WATCH_VIDEO` / `WATCH_VIDEO_ON_MOBILE`
* `PLAY_ON_DESKTOP`
* `STREAM_ON_DESKTOP`
* `PLAY_ACTIVITY`

## Usage

1. Open the **Discord Desktop App** and accept (**Enroll**) the quests you want to complete.
2. Press `Ctrl + Shift + I` (`Cmd + Option + I` on macOS) to open Developer Tools.
   * *Note: If DevTools is disabled on your client, add `"DANGEROUS_ENABLE_DEVTOOLS_ONLY_ENABLE_IF_YOU_KNOW_WHAT_YOURE_DOING": true` to `%appdata%\discord\settings.json` and restart Discord.*
3. Switch to the **Console** tab.
4. If pasting is blocked by the browser console, type `allow pasting` and press `Enter`.
5. Paste the script (`Ctrl + V`) into the console and press `Enter`.

## CODE

```javascript
(async () => {
	delete window.$;
	const wpRequire = webpackChunkdiscord_app.push([[Symbol()], {}, r => r]);
	webpackChunkdiscord_app.pop();

	const ApplicationStreamingStore = Object.values(wpRequire.c).find(x => x?.exports?.A?.__proto__?.getStreamerActiveStreamMetadata).exports.A;
	const RunningGameStore = Object.values(wpRequire.c).find(x => x?.exports?.Ay?.getRunningGames).exports.Ay;
	const QuestsStore = Object.values(wpRequire.c).find(x => x?.exports?.A?.__proto__?.getQuest).exports.A;
	const ChannelStore = Object.values(wpRequire.c).find(x => x?.exports?.A?.__proto__?.getAllThreadsForParent).exports.A;
	const GuildChannelStore = Object.values(wpRequire.c).find(x => x?.exports?.A?.__proto__?.getSFWDefaultChannel).exports.A;
	const FluxDispatcher = Object.values(wpRequire.c).find(x => x?.exports?.h?.__proto__?.flushWaitQueue).exports.h;
	const api = Object.values(wpRequire.c).find(x => x?.exports?.Bo?.get).exports.Bo;

	const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
	const randInt = (min, max) => Math.floor(Math.random() * (max - min + 1)) + min;

	const supportedTasks = ["WATCH_VIDEO", "PLAY_ON_DESKTOP", "STREAM_ON_DESKTOP", "PLAY_ACTIVITY", "WATCH_VIDEO_ON_MOBILE"];
	const isApp = typeof DiscordNative !== "undefined";
	const skippedIds = new Set();

	const getAvailableQuests = () => {
		return [...QuestsStore.quests.values()].filter(x =>
			!skippedIds.has(x.id) &&
			x.userStatus?.enrolledAt &&
			!x.userStatus?.completedAt &&
			new Date(x.config.expiresAt).getTime() > Date.now() &&
			supportedTasks.find(y => Object.keys((x.config.taskConfig ?? x.config.taskConfigV2).tasks).includes(y))
		);
	};

	if (getAvailableQuests().length === 0) {
		console.log("Nema aktivnih prihvaćenih questova.");
		return;
	}

	let isFirstQuest = true;

	while (true) {
		let available = getAvailableQuests();
		if (available.length === 0) {
			console.log("Svi prihvaćeni questovi su završeni!");
			break;
		}

		if (isFirstQuest) {
			isFirstQuest = false;
			const initialDelay = randInt(5000, 15000);
			console.log(`Pronađeno ${available.length} questova. Prvi kreće za ${Math.round(initialDelay / 1000)}s...`);
			await sleep(initialDelay);
		} else {
			// Random pauza između 10 minuta i 2 sata (120 minuta)
			const waitMs = randInt(10 * 60 * 1000, 120 * 60 * 1000);
			const nextTime = new Date(Date.now() + waitMs).toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" });
			console.log(`Pauza prije sljedećeg questa: ${Math.round(waitMs / 60000)} min (kreće oko ${nextTime}). Ostalo još: ${available.length}`);
			await sleep(waitMs);
		}

		// Ponovo učitava listu nakon pauze u slučaju da je neki quest istekao ili si prihvatio nove
		available = getAvailableQuests();
		if (available.length === 0) {
			console.log("Nema više aktivnih questova nakon pauze.");
			break;
		}

		// Nasumično bira jedan quest iz liste
		const quest = available[randInt(0, available.length - 1)];
		const pid = randInt(300, 7500) * 4;
		const questName = quest.config.messages.questName;
		const taskConfig = quest.config.taskConfig ?? quest.config.taskConfigV2;
		const taskName = supportedTasks.find(x => taskConfig.tasks[x] != null);
		const taskData = taskConfig.tasks[taskName];
		const applicationId = quest.config.application?.id ?? taskData.applications?.[0]?.id;
		const secondsNeeded = taskData.target;
		let secondsDone = quest.userStatus?.progress?.[taskName]?.value ?? 0;

		try {
			if (taskName === "WATCH_VIDEO" || taskName === "WATCH_VIDEO_ON_MOBILE") {
				console.log(`Pokrećem video zadatak: ${questName}`);
				let completed = false;
				let lastTick = Date.now();

				while (!completed && secondsDone < secondsNeeded) {
					const waitMs = randInt(8000, 14000);
					await sleep(waitMs);

					const now = Date.now();
					const realElapsed = (now - lastTick) / 1000;
					lastTick = now;

					const effectiveAdvance = Math.random() < 0.1 ? realElapsed * 0.4 : realElapsed;
					secondsDone = Math.min(secondsNeeded, secondsDone + effectiveAdvance);

					const res = await api.post({
						url: `/quests/${quest.id}/video-progress`,
						body: { timestamp: Number(secondsDone.toFixed(3)) }
					});

					completed = res.body.completed_at != null;
					console.log(`Video napredak (${questName}): ${Math.floor(secondsDone)}/${secondsNeeded}s`);
				}

				if (!completed) {
					await sleep(randInt(1500, 3500));
					await api.post({
						url: `/quests/${quest.id}/video-progress`,
						body: { timestamp: secondsNeeded }
					});
				}

				console.log(`Zadatak završen: ${questName}`);

			} else if (taskName === "PLAY_ON_DESKTOP") {
				if (!isApp) {
					console.log(`Preskačem (${questName}) - zahtijeva Discord Desktop aplikaciju.`);
					skippedIds.add(quest.id);
					isFirstQuest = true;
					continue;
				}

				const res = await api.get({ url: `/applications/public?application_ids=${applicationId}` });
				const appData = res.body[0];
				const rawExe = appData.executables?.find(x => x.os === "win32")?.name?.replace(">", "") ?? `${appData.name.replace(/[\/\\:*?"<>|]/g, "")}.exe`;
				const exeName = rawExe.split(/[\/\\]/).pop();
				const cleanRelPath = rawExe.replace(/\\/g, "/").toLowerCase();

				const fakeGame = {
					cmdLine: `"C:\\Program Files\\${appData.name}\\${rawExe.replace(/\//g, "\\")}"`,
					exeName,
					exePath: `c:/program files/${appData.name.toLowerCase()}/${cleanRelPath}`,
					hidden: false,
					isLauncher: false,
					id: applicationId,
					name: appData.name,
					pid: pid,
					pidPath: [pid],
					processName: appData.name,
					start: Date.now() - randInt(3000, 9000),
				};

				const realGames = RunningGameStore.getRunningGames();
				const realGetRunningGames = RunningGameStore.getRunningGames;
				const realGetGameForPID = RunningGameStore.getGameForPID;

				RunningGameStore.getRunningGames = () => [...realGetRunningGames.call(RunningGameStore), fakeGame];
				RunningGameStore.getGameForPID = (p) => p === pid ? fakeGame : realGetGameForPID.call(RunningGameStore, p);

				FluxDispatcher.dispatch({
					type: "RUNNING_GAMES_CHANGE",
					removed: [],
					added: [fakeGame],
					games: [...realGames, fakeGame]
				});

				console.log(`Pokrenuta simulacija igre ${appData.name}. Preostalo ~${Math.ceil((secondsNeeded - secondsDone) / 60)} min.`);

				await new Promise(resolve => {
					let finishing = false;
					const fn = async data => {
						const progress = quest.config.configVersion === 1
							? data.userStatus.streamProgressSeconds
							: Math.floor(data.userStatus.progress.PLAY_ON_DESKTOP.value);

						console.log(`Napredak igre (${appData.name}): ${progress}/${secondsNeeded}s`);

						if (progress >= secondsNeeded && !finishing) {
							finishing = true;
							FluxDispatcher.unsubscribe("QUESTS_SEND_HEARTBEAT_SUCCESS", fn);

							const lingerMs = randInt(35000, 110000);
							console.log(`Zadatak ispunjen! Ostanak u igri još ${Math.round(lingerMs / 1000)}s prije gašenja...`);
							await sleep(lingerMs);

							RunningGameStore.getRunningGames = realGetRunningGames;
							RunningGameStore.getGameForPID = realGetGameForPID;
							FluxDispatcher.dispatch({
								type: "RUNNING_GAMES_CHANGE",
								removed: [fakeGame],
								added: [],
								games: realGetRunningGames.call(RunningGameStore)
							});

							console.log(`Zadatak završen: ${questName}`);
							resolve();
						}
					};
					FluxDispatcher.subscribe("QUESTS_SEND_HEARTBEAT_SUCCESS", fn);
				});

			} else if (taskName === "STREAM_ON_DESKTOP") {
				if (!isApp) {
					console.log(`Preskačem (${questName}) - zahtijeva Discord Desktop aplikaciju.`);
					skippedIds.add(quest.id);
					isFirstQuest = true;
					continue;
				}

				const realFunc = ApplicationStreamingStore.getStreamerActiveStreamMetadata;
				ApplicationStreamingStore.getStreamerActiveStreamMetadata = () => ({
					id: applicationId,
					pid,
					sourceName: questName
				});

				console.log(`Stream postavljen na ${questName}. Pokreni bilo koji stream u VC sa još bar jednom osobom (~${Math.ceil((secondsNeeded - secondsDone) / 60)} min).`);

				await new Promise(resolve => {
					let finishing = false;
					const fn = async data => {
						const progress = quest.config.configVersion === 1
							? data.userStatus.streamProgressSeconds
							: Math.floor(data.userStatus.progress.STREAM_ON_DESKTOP.value);

						console.log(`Napredak streama (${questName}): ${progress}/${secondsNeeded}s`);

						if (progress >= secondsNeeded && !finishing) {
							finishing = true;
							FluxDispatcher.unsubscribe("QUESTS_SEND_HEARTBEAT_SUCCESS", fn);

							const lingerMs = randInt(30000, 90000);
							console.log(`Zadatak ispunjen! Održavam stream još ${Math.round(lingerMs / 1000)}s...`);
							await sleep(lingerMs);

							ApplicationStreamingStore.getStreamerActiveStreamMetadata = realFunc;
							console.log(`Zadatak završen: ${questName}`);
							resolve();
						}
					};
					FluxDispatcher.subscribe("QUESTS_SEND_HEARTBEAT_SUCCESS", fn);
				});

			} else if (taskName === "PLAY_ACTIVITY") {
				const fallbackGuild = Object.values(GuildChannelStore.getAllGuilds()).find(x => x != null && x.VOCAL?.length > 0);
				const channelId = ChannelStore.getSortedPrivateChannels()[0]?.id ?? fallbackGuild?.VOCAL?.[0]?.channel?.id;

				if (!channelId) {
					console.log(`Preskačem (${questName}) - nije pronađen nijedan kanal za PLAY_ACTIVITY.`);
					skippedIds.add(quest.id);
					isFirstQuest = true;
					continue;
				}

				const streamKey = `call:${channelId}:1`;
				console.log(`Pokrećem aktivnost: ${questName}`);

				while (true) {
					const res = await api.post({
						url: `/quests/${quest.id}/heartbeat`,
						body: { stream_key: streamKey, terminal: false }
					});
					const progress = res.body.progress.PLAY_ACTIVITY.value;
					console.log(`Napredak aktivnosti (${questName}): ${progress}/${secondsNeeded}s`);

					if (progress >= secondsNeeded) {
						await sleep(randInt(4000, 10000));
						await api.post({
							url: `/quests/${quest.id}/heartbeat`,
							body: { stream_key: streamKey, terminal: true }
						});
						break;
					}

					await sleep(randInt(19500, 23500));
				}

				console.log(`Zadatak završen: ${questName}`);
			}
		} catch (err) {
			console.error(`Greška kod questa ${questName}, pokušavam sljedeći nakon pauze:`, err);
		}
	}
})();
```

## Important Notes

* **Keep Client Open:** Because the script spaces out quests with randomized breaks, keep the Discord desktop app open and ensure your PC does not enter **Sleep mode** while it is running.
* **Stream Quests:** For `STREAM_ON_DESKTOP` tasks, manually join a voice channel with at least one other user and stream any window while the script is active.
* **Stop Execution:** Press `Ctrl + R` inside Discord at any time to reload the client and immediately stop the script.
