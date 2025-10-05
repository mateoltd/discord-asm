#!/usr/bin/env node

/**
 * Ralph Loop Supervisor - Automated Copilot CLI Development Loop
 *
 * This script maintains a persistent Copilot CLI session and automatically
 * drives development iterations based on the PROMPT.md template.
 *
 * Features:
 * - Monitors for RALPH_NEXT sentinel to detect iteration completion
 * - Automatically sends next iteration prompts
 * - Handles inactivity with /continue commands
 * - Auto-commits and pushes changes after each iteration
 * - Restarts Copilot CLI on crashes with proper seeding
 * - Cross-platform logging to .ralph/cli.log
 */

const os = require('os');
const fs = require('fs');
const path = require('path');
const { spawn, exec } = require('child_process');

// Configuration (with environment variable support)
const CONFIG = {
  COPILOT_CMD: process.env.RALPH_COPILOT_CMD || 'copilot',
  LOG_FILE: path.join('.ralph', 'cli.log'),
  MAX_LOG_SIZE: 10 * 1024 * 1024, // 10MB
  RALPH_NEXT_SENTINEL: 'RALPH_NEXT',
  RESTART_DELAY: 5000, // 5 seconds for failed prompts
  ITERATION_PROMPT: `Begin the NEXT iteration using the Iteration template in PROMPT.md.
Focus on the smallest high-value increment.
End the iteration with: RALPH_NEXT
`
};

class RalphSupervisor {
  constructor() {
    this.lastActivity = Date.now();
    this.iterationCount = 0;
    this.isShuttingDown = false;

    // Ensure log directory exists
    const logDir = path.dirname(CONFIG.LOG_FILE);
    if (!fs.existsSync(logDir)) {
      fs.mkdirSync(logDir, { recursive: true });
    }

    // Set up signal handlers for graceful shutdown
    process.on('SIGINT', () => this.shutdown('SIGINT'));
    process.on('SIGTERM', () => this.shutdown('SIGTERM'));

    this.log('Ralph Loop Supervisor starting...');

    // Seed the session first, then start iterations
    this.seedCopilotSession().then(() => {
      this.log('Session seeded successfully, starting iterations...');
      // Start the first iteration after seeding
      setTimeout(() => {
        if (!this.isShuttingDown) {
          this.startNextIteration();
        }
      }, 1000);
    }).catch((error) => {
      this.log(`Failed to seed session: ${error.message}`, 'ERROR');
      this.log('Continuing with iterations anyway...');
      // Start iterations even if seeding fails
      setTimeout(() => {
        if (!this.isShuttingDown) {
          this.startNextIteration();
        }
      }, 1000);
    });
  }

  log(message, level = 'INFO') {
    const timestamp = new Date().toISOString();
    const logEntry = `[${timestamp}] [${level}] ${message}\n`;

    // Console output for visibility
    console.log(logEntry.trim());

    // Append to log file
    try {
      fs.appendFileSync(CONFIG.LOG_FILE, logEntry);
      this.rotateLogIfNeeded();
    } catch (error) {
      console.error(`Failed to write to log file: ${error.message}`);
    }
  }

  rotateLogIfNeeded() {
    try {
      const stats = fs.statSync(CONFIG.LOG_FILE);
      if (stats.size > CONFIG.MAX_LOG_SIZE) {
        const backupPath = `${CONFIG.LOG_FILE}.${Date.now()}.bak`;
        fs.renameSync(CONFIG.LOG_FILE, backupPath);
        this.log(`Log rotated to ${backupPath}`);
      }
    } catch (error) {
      // Ignore stat errors
    }
  }

  startCopilot() {
    this.log(`Copilot CLI ready for prompt execution`);
    // No need to start a persistent process - we'll execute prompts directly
  }

  executePrompt(prompt) {
    return new Promise((resolve, reject) => {
      this.log(`Executing prompt: ${prompt.substring(0, 50)}...`);

      const args = ['-p', prompt, '--allow-all-tools'];
      if (CONFIG.COPILOT_CMD !== 'copilot') {
        args.unshift(CONFIG.COPILOT_CMD);
      }

      const copilotProcess = spawn(CONFIG.COPILOT_CMD, args, {
        cwd: process.cwd(),
        stdio: ['ignore', 'pipe', 'pipe'],
        env: process.env
      });

      let stdout = '';
      let stderr = '';

      copilotProcess.stdout.on('data', (data) => {
        stdout += data.toString();
        this.log(`COPILOT: ${data.toString().trim()}`);
      });

      copilotProcess.stderr.on('data', (data) => {
        stderr += data.toString();
        this.log(`COPILOT STDERR: ${data.toString().trim()}`);
      });

      copilotProcess.on('exit', (code, signal) => {
        if (code === 0) {
          this.log('Prompt execution completed successfully');
          resolve(stdout);
        } else {
          const error = `Prompt execution failed (exit code ${code}): ${stderr}`;
          this.log(error, 'ERROR');
          reject(new Error(error));
        }
      });

      copilotProcess.on('error', (error) => {
        this.log(`Failed to execute prompt: ${error.message}`, 'ERROR');
        reject(error);
      });
    });
  }


  seedCopilotSession() {
    this.log('Seeding Copilot session with initial commands');

    // Execute initial setup prompt
    const setupPrompt = 'Load PROMPT.md and prepare for iterative development. Wait for further instructions.';
    return this.executePrompt(setupPrompt);
  }

  startNextIteration() {
    this.iterationCount++;
    this.log(`Starting iteration ${this.iterationCount}`);

    // First auto-commit the previous iteration
    this.autoCommitChanges().then(() => {
      // Then execute the next iteration prompt
      this.executeIterationPrompt();
    }).catch((error) => {
      this.log(`Auto-commit failed: ${error.message}`, 'ERROR');
      // Continue with iteration even if commit fails
      this.executeIterationPrompt();
    });
  }

  executeIterationPrompt() {
    const prompt = CONFIG.ITERATION_PROMPT;
    this.executePrompt(prompt).then((output) => {
      // Check if the output contains RALPH_NEXT
      if (output.includes(CONFIG.RALPH_NEXT_SENTINEL)) {
        this.log('Detected RALPH_NEXT sentinel - starting next iteration');
        setTimeout(() => {
          this.startNextIteration();
        }, 1000); // Brief delay before next iteration
      } else {
        this.log('Iteration completed but RALPH_NEXT not found - waiting for manual trigger or timeout');
      }
    }).catch((error) => {
      this.log(`Iteration prompt execution failed: ${error.message}`, 'ERROR');
      // Schedule restart after failure
      setTimeout(() => {
        if (!this.isShuttingDown) {
          this.startNextIteration();
        }
      }, CONFIG.RESTART_DELAY);
    });
  }


  async autoCommitChanges() {
    return new Promise((resolve, reject) => {
      this.log('Auto-committing changes');

      const timestamp = new Date().toISOString().replace(/[:.]/g, '-').slice(0, -5) + 'Z';
      const commitMessage = `ralph: iteration ${timestamp}`;

      // Run git commands sequentially
      const commands = [
        ['git', ['add', '-A']],
        ['git', ['commit', '-m', commitMessage]],
        ['git', ['push', 'origin', 'main']]
      ];

      let currentIndex = 0;

      const runNext = () => {
        if (currentIndex >= commands.length) {
          this.log('Auto-commit completed successfully');
          resolve();
          return;
        }

        const [cmd, args] = commands[currentIndex];
        const child = spawn(cmd, args, {
          cwd: process.cwd(),
          stdio: ['ignore', 'pipe', 'pipe']
        });

        let stdout = '';
        let stderr = '';

        child.stdout.on('data', (data) => { stdout += data; });
        child.stderr.on('data', (data) => { stderr += data; });

        child.on('exit', (code) => {
          if (code === 0) {
            currentIndex++;
            runNext();
          } else {
            const error = `Command failed: ${cmd} ${args.join(' ')} (exit ${code})\n${stderr}`;
            reject(new Error(error));
          }
        });

        child.on('error', (error) => {
          reject(new Error(`Failed to run ${cmd}: ${error.message}`));
        });
      };

      runNext();
    });
  }

  shutdown(signal) {
    this.log(`Received ${signal}, shutting down gracefully...`);
    this.isShuttingDown = true;
    process.exit(0);
  }
}

// Start the supervisor
if (require.main === module) {
  new RalphSupervisor();
}

module.exports = RalphSupervisor;
