'use strict';

const vscode = require('vscode');

function activate(context) {
    function provideFoldingRanges(document) {
        const ranges = [];
        const stack = [];

        for (let line = 0; line < document.lineCount; line += 1) {
            const text = document.lineAt(line).text;

            for (const ch of text) {
                if (ch === '{') {
                    stack.push(line);
                } else if (ch === '}' && stack.length > 0) {
                    const start = stack.pop();

                    if (start !== undefined && start < line) {
                        ranges.push(new vscode.FoldingRange(start, line));
                    }
                }
            }
        }

        return ranges;
    }

    context.subscriptions.push(
        vscode.languages.registerFoldingRangeProvider(
            { language: 'xero' },
            { provideFoldingRanges }
        )
    );
}

function deactivate() {}

module.exports = { activate, deactivate };
