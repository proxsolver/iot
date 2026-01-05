/**
 * Example Usage of SEO Blog Content Agent
 * Demonstrates how to use the agent for IoT tutorial optimization
 */

const SEOTechnicalBlogAgent = require('./seo-blog-agent');

// Initialize the agent
const agent = new SEOTechnicalBlogAgent();

// Example 1: Analyze a sample Arduino Nicla Vision tutorial
console.log('=== Example 1: Content Analysis ===\n');

const sampleContent = `
# Arduino Nicla Vision Tutorial: Complete Guide

Learn how to use the Arduino Nicla Vision board for computer vision projects and machine learning at the edge.

## What is Arduino Nicla Vision?

The Arduino Nicla Vision is a powerful board for IoT applications with computer vision capabilities. It features the STM32H747 processor and built-in camera module.

## Hardware Requirements

- Arduino Nicla Vision board
- USB-C cable
- Computer with Arduino IDE 2.0+

## Software Setup

Install the Arduino IDE and add the Nicla Vision board support.

## Getting Started

Upload your first sketch to test the camera functionality.

## Conclusion

The Arduino Nicla Vision is perfect for edge AI projects and computer vision applications.
`;

const keywords = {
  primary: ['Arduino Nicla Vision'],
  secondary: ['computer vision', 'IoT camera', 'edge AI', 'machine learning'],
  longTail: ['Arduino Nicla Vision projects', 'TinyML tutorial', 'edge computing IoT']
};

const analysis = agent.analyzeContent(sampleContent, keywords);

console.log('Title Analysis:');
console.log(JSON.stringify(analysis.title, null, 2));
console.log('\n');

console.log('Heading Analysis:');
console.log(JSON.stringify(analysis.headings, null, 2));
console.log('\n');

console.log('Keyword Analysis:');
console.log(JSON.stringify(analysis.keywords, null, 2));
console.log('\n');

console.log('Readability Analysis:');
console.log(JSON.stringify(analysis.readability, null, 2));
console.log('\n');

console.log('Structure Analysis:');
console.log(JSON.stringify(analysis.structure, null, 2));
console.log('\n');

// Example 2: Generate title variants
console.log('=== Example 2: Title Variants ===\n');

const titleVariants = agent.generateTitleVariants(
  'Object Detection System',
  'Arduino Nicla Vision',
  'beginner'
);

console.log('Generated Title Variants:\n');
titleVariants.forEach((variant, index) => {
  const status = variant.length >= 50 && variant.length <= 60 ? '✓' : '✗';
  console.log(`${index + 1}. ${status} ${variant.title}`);
  console.log(`   Length: ${variant.length} chars | Words: ${variant.wordCount}\n`);
});

// Example 3: Generate meta information
console.log('=== Example 3: Meta Information ===\n');

const metaInfo = agent.generateMetaSuggestions(sampleContent, keywords);
console.log('Suggested Meta Title:');
console.log(metaInfo.metaTitle);
console.log(`\nLength: ${metaInfo.metaTitle.length} characters\n`);

console.log('Suggested Meta Description:');
console.log(metaInfo.metaDescription);
console.log(`\nLength: ${metaInfo.metaDescription.length} characters\n`);

console.log('Meta Optimization Suggestions:');
metaInfo.suggestions.forEach(suggestion => {
  console.log(`- ${suggestion}`);
});
console.log('\n');

// Example 4: Generate social media summary
console.log('=== Example 4: Social Media Summary ===\n');

const twitterSummary = agent.generateSocialSummary(sampleContent, 280);
console.log('Twitter Summary (280 chars):');
console.log(twitterSummary);
console.log(`\nLength: ${twitterSummary.length} characters\n`);

const linkedinSummary = agent.generateSocialSummary(sampleContent, 1500);
console.log('LinkedIn Summary (1500 chars):');
console.log(linkedinSummary);
console.log(`\nLength: ${linkedinSummary.length} characters\n`);

// Example 5: Priority recommendations
console.log('=== Example 5: Priority Recommendations ===\n');

if (analysis.recommendations.length > 0) {
  console.log(`Found ${analysis.recommendations.length} recommendations:\n`);

  analysis.recommendations.forEach((rec, index) => {
    const priorityIcon = rec.priority === 'high' ? '🔴' : rec.priority === 'medium' ? '🟡' : '🟢';
    console.log(`${index + 1}. ${priorityIcon} [${rec.priority.toUpperCase()}] ${rec.category}`);
    console.log(`   Issue: ${rec.issue}`);
    console.log(`   Suggestion: ${rec.suggestion}\n`);
  });
} else {
  console.log('✓ No critical issues found! Content is well optimized.\n');
}

// Example 6: SEO Checklist validation
console.log('=== Example 6: SEO Checklist Validation ===\n');

const seoChecklist = require('./seo-checklist.json');
const checklist = seoChecklist.seoChecklist.categories;

console.log('SEO Checklist Summary:\n');
checklist.forEach(category => {
  const totalItems = category.items.length;
  const highPriorityItems = category.items.filter(item => item.priority === 'high').length;

  console.log(`${category.category}:`);
  console.log(`  Total items: ${totalItems}`);
  console.log(`  High priority: ${highPriorityItems}`);
  console.log(`  Items: ${category.items.map(item => item.task).join(', ')}\n`);
});

// Example 7: Keyword research lookup
console.log('=== Example 7: Keyword Research ===\n');

const keywordResearch = require('./iot-keyword-research.json');
const topics = keywordResearch.keywordResearch.topics;

console.log('Available Keyword Topics:\n');
Object.keys(topics).forEach(topicName => {
  const topic = topics[topicName];
  console.log(`${topicName}:`);
  console.log(`  Primary Keywords: ${topic.primaryKeywords.length}`);
  console.log(`  Secondary Keywords: ${topic.secondaryKeywords.length}`);
  console.log(`  Long-tail Keywords: ${topic.longTailKeywords.length}`);
  console.log(`  Search Volume: ${topic.volume}`);
  console.log(`  Trending: ${topic.trending}\n`);
});

// Example 8: Content improvement suggestions
console.log('=== Example 8: Content Improvement Suggestions ===\n');

console.log('Improvements to make sample content SEO-optimized:\n');

// Analyze what's missing
const improvements = [];

// Check content length
if (sampleContent.split(/\s+/).length < 1500) {
  improvements.push({
    issue: 'Content too short',
    suggestion: 'Expand content to 1500+ words for comprehensive coverage',
    currentWords: sampleContent.split(/\s+/).length,
    targetWords: 1500
  });
}

// Check for code examples
if (!sampleContent.includes('```')) {
  improvements.push({
    issue: 'Missing code examples',
    suggestion: 'Add complete, syntax-highlighted code examples for each step'
  });
}

// Check for troubleshooting
if (!sampleContent.toLowerCase().includes('troubleshoot')) {
  improvements.push({
    issue: 'No troubleshooting section',
    suggestion: 'Add troubleshooting section with common issues and solutions'
  });
}

// Check for links
if (!sampleContent.includes('http')) {
  improvements.push({
    issue: 'No external links',
    suggestion: 'Add links to official documentation and related resources'
  });
}

improvements.forEach((improvement, index) => {
  console.log(`${index + 1}. ${improvement.issue}`);
  console.log(`   Suggestion: ${improvement.suggestion}`);
  if (improvement.currentWords) {
    console.log(`   Current: ${improvement.currentWords} words | Target: ${improvement.targetWords} words`);
  }
  console.log('');
});

console.log('=== Analysis Complete ===\n');

// Export analysis results
console.log('To save analysis results:');
console.log('const fs = require("fs");');
console.log('fs.writeFileSync("analysis-results.json", JSON.stringify(analysis, null, 2));\n');
